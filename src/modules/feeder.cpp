#include "feeder.h"
#include "../config.h"
#include "mqtt_manager.h"

Servo Feeder::_servo;
uint16_t Feeder::_servingSize = DEFAULT_SERVING_SIZE;
bool Feeder::_dispensing = false;
unsigned long Feeder::_dispenseStartTime = 0;

void Feeder::init() {
    pinMode(SERVO_POWER_PIN, OUTPUT);
    digitalWrite(SERVO_POWER_PIN, LOW);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    if (!_servo.attached()) {
        _servo.attach(SERVO_PIN, SERVO_MIN, SERVO_MAX);
        Serial.println("Servo attached");
    }
    setServo(SERVO_STOP);
}

void Feeder::update() {
    if (_dispensing) {
        if (millis() - _dispenseStartTime >= _servingSize) {
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
    _servingSize = size;
    Serial.printf("Serving size updated to %d ms\n", _servingSize);
    sendStatus();
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
