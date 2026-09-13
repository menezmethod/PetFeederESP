#include "scheduler.h"
#include "../config.h"
#include "mqtt_manager.h"
#include "feeder.h"
#include "../utils/time_utils.h"

Scheduler::Schedule Scheduler::_schedules[MAX_SCHEDULES] = DEFAULT_SCHEDULES;
int Scheduler::_scheduleCount = DEFAULT_SCHEDULE_COUNT;
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
                uint8_t todayMask = 1 << timeinfo.tm_wday;
                for (int i = 0; i < _scheduleCount; i++) {
                    if (_schedules[i].enabled &&
                        (_schedules[i].days & todayMask) != 0 &&
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
    StaticJsonDocument<512> doc;
    deserializeJson(doc, message);

    JsonArray scheduleArray = doc["schedules"];

    // The app sends its full current list each time (add/remove is just
    // "send a longer or shorter array"), not an incremental add/remove
    // command -- one code path handles every case.
    _scheduleCount = min((size_t)MAX_SCHEDULES, scheduleArray.size());
    for (int i = 0; i < _scheduleCount; i++) {
        JsonObject scheduleObj = scheduleArray[i];
        int hour = scheduleObj["hour"];
        int minute = scheduleObj["minute"];
        // Clamp at this trust boundary (MQTT payload) -- an out-of-range value
        // should fail safe (never match, never fire) rather than compare against
        // undefined tm_hour/tm_min ranges.
        _schedules[i].hour = constrain(hour, 0, 23);
        _schedules[i].minute = constrain(minute, 0, 59);
        _schedules[i].enabled = scheduleObj["enabled"];
        _schedules[i].days = scheduleObj["days"] | SCHEDULE_ALL_DAYS;
    }

    Serial.printf("Schedule updated: %d slot(s)\n", _scheduleCount);
    saveSchedules();
}

void Scheduler::sendScheduleStatus() {
    StaticJsonDocument<512> doc;
    doc["enabled"] = _enabled;
    JsonArray scheduleArray = doc.createNestedArray("schedules");
    for (int i = 0; i < _scheduleCount; i++) {
        JsonObject scheduleObj = scheduleArray.createNestedObject();
        scheduleObj["hour"] = _schedules[i].hour;
        scheduleObj["minute"] = _schedules[i].minute;
        scheduleObj["enabled"] = _schedules[i].enabled;
        scheduleObj["days"] = _schedules[i].days;
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
    _preferences.putInt("count", _scheduleCount);
    _preferences.putBool("enabled", _enabled);
    _preferences.end();
}

void Scheduler::loadSchedules() {
    _preferences.begin("sched_cfg", true);
    if (_preferences.isKey("schedules") && _preferences.getBytesLength("schedules") == sizeof(_schedules)) {
        _preferences.getBytes("schedules", _schedules, sizeof(_schedules));
        _scheduleCount = constrain(_preferences.getInt("count", DEFAULT_SCHEDULE_COUNT), 1, MAX_SCHEDULES);
        // Length check only proves the blob is the right size, not that its
        // contents are sane -- clamp the same way parseSchedule() does, in
        // case flash corruption ever produces an out-of-range value.
        for (int i = 0; i < MAX_SCHEDULES; i++) {
            _schedules[i].hour = constrain(_schedules[i].hour, 0, 23);
            _schedules[i].minute = constrain(_schedules[i].minute, 0, 59);
        }
    }
    _enabled = _preferences.getBool("enabled", true);
    _preferences.end();
    Serial.printf("Loaded %d schedule slot(s), scheduling %s\n", _scheduleCount, _enabled ? "enabled" : "disabled");
}
