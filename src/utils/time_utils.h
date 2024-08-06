#pragma once

#include <Arduino.h>
#include <time.h>

class TimeUtils {
public:
    static void syncTime();
    static bool getLocalTime(struct tm* timeinfo);
    static String getFormattedTime();

private:
    static bool _timeSynced;
};
