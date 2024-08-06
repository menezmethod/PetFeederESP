#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

class Feeder {
public:
    static void init();

    static void update();

    static void dispense();

    static void setServingSize(uint16_t size);

    static void sendStatus();

    static void cleanup();

private:
    static void setServo(uint16_t duty);

    static Servo _servo;
    static uint16_t _servingSize;
    static bool _dispensing;
    static unsigned long _dispenseStartTime;
};
