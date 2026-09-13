#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <Preferences.h>

class Feeder {
public:
    enum class FeedTrigger { Manual, Scheduled, Button };

    static void init();

    static void update();

    static void dispense(FeedTrigger trigger = FeedTrigger::Manual);

    static void setServingSize(uint16_t size);

    static void sendStatus();

    static bool isDispensing();

    static void pollButton();

private:
    static void setServo(uint16_t duty);
    static void saveServingSize();
    static void loadServingSize();
    static void publishLastFed();
    static const char* triggerName(FeedTrigger t);

    static Servo _servo;
    static uint16_t _servingSize;
    static bool _dispensing;
    static unsigned long _dispenseStartTime;
    static FeedTrigger _currentTrigger;
    static Preferences _preferences;

    static int _lastRawButtonState;
    static int _lastStableButtonState;
    static unsigned long _lastButtonDebounceTime;
};
