#include "feeder.h"
#include "../config.h"
#include "mqtt_manager.h"

Servo Feeder::_servo;
uint16_t Feeder::_servingSize = DEFAULT_SERVING_SIZE;
bool Feeder::_dispensing = false;
unsigned long Feeder::_dispenseStartTime = 0;
Preferences Feeder::_preferences;

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
}

void Feeder::update() {
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
        }
    }
}

void Feeder::dispense() {
    if (!_dispensing) {
        Serial.printf("Dispensing for %d ms\n", _servingSize);
        digitalWrite(SERVO_POWER_PIN, HIGH);
        delay(200);  // Wait for power stabilization
        setServo(SERVO_MAX);
        _dispensing = true;
        _dispenseStartTime = millis();
    }
}

void Feeder::setServingSize(uint16_t size) {
    _servingSize = min((unsigned long)size, (unsigned long)MAX_DISPENSE_DURATION_MS);
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

void Feeder::sendStatus() {
    DynamicJsonDocument doc(256);
    doc["servingSize"] = _servingSize;
    String jsonString;
    serializeJson(doc, jsonString);
    MQTTManager::publish(TOPIC_STATUS, jsonString.c_str());
}

void Feeder::setServo(uint16_t duty) {
    Serial.printf("Setting servo to %d\n", duty);
    _servo.writeMicroseconds(duty);
}

void Feeder::cleanup() {
    _servo.detach();
    digitalWrite(SERVO_POWER_PIN, LOW);
}
