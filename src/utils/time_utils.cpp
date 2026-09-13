#include "time_utils.h"
#include "../config.h"

bool TimeUtils::_timeSynced = false;

bool TimeUtils::syncTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for time sync");
    unsigned long start = millis();
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        if (millis() - start > NTP_SYNC_TIMEOUT_MS) {
            // Bounded: an unreachable NTP server (no internet, firewalled LAN)
            // must never hang setup() forever. Caller retries later.
            Serial.println("\nNTP sync timed out, will retry");
            return false;
        }
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
    }
    return true;
}

bool TimeUtils::isSynced() {
    return _timeSynced;
}

time_t TimeUtils::getEpoch() {
    return time(nullptr);
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
