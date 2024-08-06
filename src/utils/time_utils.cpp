#include "time_utils.h"

bool TimeUtils::_timeSynced = false;

void TimeUtils::syncTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for time sync");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    _timeSynced = true;
    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
    tzset();
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.printf("Time synchronized: %s", asctime(&timeinfo));
    } else {
        Serial.println("Failed to obtain time");
    }
}

bool TimeUtils::getLocalTime(struct tm *timeinfo) {
    if (!timeinfo) {
        return false;
    }
    time_t now = time(nullptr);
    if (now < 8 * 3600 * 2) {
        return false;
    }
    localtime_r(&now, timeinfo);
    return true;
}

String TimeUtils::getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Time not set";
    }
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    return String(timeStringBuff);
}
