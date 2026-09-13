#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "../config.h"

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
        uint8_t days;  // bitmask, bit0=Sunday...bit6=Saturday (matches tm_wday)
    };

private:
    static void saveSchedules();
    static void loadSchedules();

    static Schedule _schedules[MAX_SCHEDULES];
    static int _scheduleCount;  // how many of _schedules[] are actually in use, 1..MAX_SCHEDULES
    static bool _enabled;
    static Preferences _preferences;
};
