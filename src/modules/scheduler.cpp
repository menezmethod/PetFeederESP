#include "scheduler.h"
#include "../config.h"
#include "mqtt_manager.h"
#include "feeder.h"
#include "../utils/time_utils.h"

Scheduler::Schedule Scheduler::_schedules[2] = DEFAULT_SCHEDULES;
bool Scheduler::_enabled = true;
Preferences Scheduler::_preferences;

void Scheduler::init() {
    TimeUtils::syncTime();
    loadSchedules();
}

void Scheduler::update() {
    if (!TimeUtils::isSynced()) {
        // syncTime() in init() is bounded and can fail (no internet yet at boot).
        // Retry periodically rather than leaving schedules permanently dead.
        static unsigned long lastSyncAttempt = 0;
        unsigned long now = millis();
        if (now - lastSyncAttempt > 30000) {
            lastSyncAttempt = now;
            TimeUtils::syncTime();
        }
        return;
    }

    static unsigned long lastCheck = 0;
    unsigned long now = millis();
    if (now - lastCheck > 10000) {
        lastCheck = now;
        struct tm timeinfo;
        if (TimeUtils::getLocalTime(&timeinfo)) {
            if (_enabled) {
                for (int i = 0; i < 2; i++) {
                    if (_schedules[i].enabled &&
                        timeinfo.tm_hour == _schedules[i].hour &&
                        timeinfo.tm_min == _schedules[i].minute &&
                        timeinfo.tm_sec < 10) {
                        Serial.println("Scheduled feeding time");
                        Feeder::dispense(Feeder::FeedTrigger::Scheduled);
                        break;
                    }
                }
            }
        }
    }
}

void Scheduler::parseSchedule(const String &message) {
    StaticJsonDocument<256> doc;
    deserializeJson(doc, message);

    JsonArray scheduleArray = doc["schedules"];

    for (int i = 0; i < 2 && i < scheduleArray.size(); i++) {
        JsonObject scheduleObj = scheduleArray[i];
        int hour = scheduleObj["hour"];
        int minute = scheduleObj["minute"];
        // Clamp at this trust boundary (MQTT payload) -- an out-of-range value
        // should fail safe (never match, never fire) rather than compare against
        // undefined tm_hour/tm_min ranges.
        _schedules[i].hour = constrain(hour, 0, 23);
        _schedules[i].minute = constrain(minute, 0, 59);
        _schedules[i].enabled = scheduleObj["enabled"];
    }

    Serial.printf("Schedule updated: %02d:%02d (%s), %02d:%02d (%s)\n",
                  _schedules[0].hour, _schedules[0].minute, _schedules[0].enabled ? "ON" : "OFF",
                  _schedules[1].hour, _schedules[1].minute, _schedules[1].enabled ? "ON" : "OFF");
    saveSchedules();
}

void Scheduler::sendScheduleStatus() {
    StaticJsonDocument<256> doc;
    doc["enabled"] = _enabled;
    JsonArray scheduleArray = doc.createNestedArray("schedules");
    for (int i = 0; i < 2; i++) {
        JsonObject scheduleObj = scheduleArray.createNestedObject();
        scheduleObj["hour"] = _schedules[i].hour;
        scheduleObj["minute"] = _schedules[i].minute;
        scheduleObj["enabled"] = _schedules[i].enabled;
    }
    String jsonString;
    serializeJson(doc, jsonString);
    MQTTManager::publish(TOPIC_SCHEDULE_STATUS, jsonString.c_str());
}

void Scheduler::setEnabled(bool enabled) {
    _enabled = enabled;
    Serial.printf("Global scheduling %s\n", _enabled ? "enabled" : "disabled");
    saveSchedules();
}

bool Scheduler::isEnabled() {
    return _enabled;
}

void Scheduler::saveSchedules() {
    _preferences.begin("sched_cfg", false);
    _preferences.putBytes("schedules", _schedules, sizeof(_schedules));
    _preferences.putBool("enabled", _enabled);
    _preferences.end();
}

void Scheduler::loadSchedules() {
    _preferences.begin("sched_cfg", true);
    if (_preferences.isKey("schedules") && _preferences.getBytesLength("schedules") == sizeof(_schedules)) {
        _preferences.getBytes("schedules", _schedules, sizeof(_schedules));
    }
    _enabled = _preferences.getBool("enabled", true);
    _preferences.end();
    Serial.printf("Loaded schedule: %02d:%02d (%s), %02d:%02d (%s), scheduling %s\n",
                  _schedules[0].hour, _schedules[0].minute, _schedules[0].enabled ? "ON" : "OFF",
                  _schedules[1].hour, _schedules[1].minute, _schedules[1].enabled ? "ON" : "OFF",
                  _enabled ? "enabled" : "disabled");
}