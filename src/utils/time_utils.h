#pragma once

#include <Arduino.h>
#include <time.h>

class TimeUtils {
public:
    static bool syncTime();
    static bool getLocalTime(struct tm* timeinfo);
    static String getFormattedTime();
    static bool isSynced();
    static time_t getEpoch();

private:
    static bool _timeSynced;
};
