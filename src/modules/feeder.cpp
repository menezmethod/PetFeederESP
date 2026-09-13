#include "feeder.h"
#include "../config.h"
#include "mqtt_manager.h"
#include "../utils/time_utils.h"

Servo Feeder::_servo;
uint16_t Feeder::_servingSize = DEFAULT_SERVING_SIZE;
bool Feeder::_dispensing = false;
unsigned long Feeder::_dispenseStartTime = 0;
Feeder::FeedTrigger Feeder::_currentTrigger = Feeder::FeedTrigger::Manual;
Preferences Feeder::_preferences;
int Feeder::_lastRawButtonState = HIGH;
int Feeder::_lastStableButtonState = HIGH;
unsigned long Feeder::_lastButtonDebounceTime = 0;

void Feeder::init() {
    pinMode(SERVO_POWER_PIN, OUTPUT);
    digitalWrite(SERVO_POWER_PIN, LOW);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    if (!_servo.attached()) {
        _servo.attach(SERVO_PIN, SERVO_MIN, SERVO_MAX);
        Serial.println("Servo attached");
    }
    setServo(SERVO_STOP);

    loadServingSize();
    _lastRawButtonState = digitalRead(BUTTON_PIN);
    _lastStableButtonState = _lastRawButtonState;
}

void Feeder::update() {
    pollButton();

    if (_dispensing) {
        // Hard ceiling independent of _servingSize/reconnect timing -- the servo
        // must never be able to run longer than this no matter what upstream
        // code does (a stalled MQTT reconnect used to block this entirely).
        unsigned long runDuration = min((unsigned long)_servingSize, (unsigned long)MAX_DISPENSE_DURATION_MS);
        if (millis() - _dispenseStartTime >= runDuration) {
            setServo(SERVO_STOP);
            delay(100);
            digitalWrite(SERVO_POWER_PIN, LOW);
            _dispensing = false;
            Serial.println("Dispense complete");
            publishLastFed();
        }
    }
}

void Feeder::pollButton() {
    // Standard edge-triggered debounce: BUTTON_PIN was configured but never
    // read anywhere in the original firmware, so the physical manual-feed
    // button the README describes has never actually worked.
    int reading = digitalRead(BUTTON_PIN);
    unsigned long now = millis();
    if (reading != _lastRawButtonState) {
        _lastButtonDebounceTime = now;
        _lastRawButtonState = reading;
    }
    if (now - _lastButtonDebounceTime > BUTTON_DEBOUNCE_MS) {
        if (reading == LOW && _lastStableButtonState == HIGH) {
            dispense(FeedTrigger::Button);
        }
        _lastStableButtonState = reading;
    }
}

void Feeder::dispense(FeedTrigger trigger) {
    if (!_dispensing) {
        _currentTrigger = trigger;
        Serial.printf("Dispensing for %d ms (trigger=%s)\n", _servingSize, triggerName(trigger));
        digitalWrite(SERVO_POWER_PIN, HIGH);
        delay(200);  // Wait for power stabilization
        setServo(SERVO_MAX);
        _dispensing = true;
        _dispenseStartTime = millis();
    }
}

const char* Feeder::triggerName(FeedTrigger t) {
    switch (t) {
        case FeedTrigger::Scheduled: return "scheduled";
        case FeedTrigger::Button: return "button";
        default: return "manual";
    }
}

void Feeder::publishLastFed() {
    // Retained: a freshly-opened app must see the last feed immediately,
    // not wait for the next one to happen while it's connected.
    StaticJsonDocument<128> doc;
    // A feed (especially button-triggered) can happen before NTP has synced
    // -- getEpoch() would otherwise publish a near-zero 1970 timestamp the
    // app would render as a real date.
    doc["fedAt"] = TimeUtils::isSynced() ? (uint32_t)TimeUtils::getEpoch() : 0;
    doc["servingSize"] = _servingSize;
    doc["trigger"] = triggerName(_currentTrigger);
    String jsonString;
    serializeJson(doc, jsonString);
    MQTTManager::publish(TOPIC_LAST_FED, jsonString.c_str(), true);
}

void Feeder::setServingSize(uint16_t size) {
    _servingSize = constrain(size, MIN_DISPENSE_DURATION_MS, MAX_DISPENSE_DURATION_MS);
    Serial.printf("Serving size updated to %d ms\n", _servingSize);
    saveServingSize();
    sendStatus();
}

void Feeder::saveServingSize() {
    _preferences.begin("feeder_cfg", false);
    _preferences.putUShort("servingSize", _servingSize);
    _preferences.end();
}

void Feeder::loadServingSize() {
    _preferences.begin("feeder_cfg", true);
    _servingSize = _preferences.getUShort("servingSize", DEFAULT_SERVING_SIZE);
    _preferences.end();
    Serial.printf("Loaded serving size: %d ms\n", _servingSize);
}

bool Feeder::isDispensing() {
    return _dispensing;
}

void Feeder::sendStatus() {
    StaticJsonDocument<128> doc;
    doc["servingSize"] = _servingSize;
    String jsonString;
    serializeJson(doc, jsonString);
    MQTTManager::publish(TOPIC_STATUS, jsonString.c_str());
}

void Feeder::setServo(uint16_t duty) {
    Serial.printf("Setting servo to %d\n", duty);
    _servo.writeMicroseconds(duty);
}
