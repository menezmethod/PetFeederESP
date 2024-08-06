#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

class Scheduler {
public:
    static void init();

    static void update();

    static void parseSchedule(const String &message);

    static void sendScheduleStatus();

    static void setEnabled(bool enabled);

    static bool isEnabled();

    struct Schedule {
        int hour, minute;
        bool enabled;
    };

private:
    static Schedule _schedules[2];
    static bool _enabled;
};
