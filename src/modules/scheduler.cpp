#include "scheduler.h"
#include "../config.h"
#include "mqtt_manager.h"
#include "feeder.h"
#include "../utils/time_utils.h"

Scheduler::Schedule Scheduler::_schedules[2] = DEFAULT_SCHEDULES;
bool Scheduler::_enabled = true;

void Scheduler::init() {
    TimeUtils::syncTime();
}

void Scheduler::update() {
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
                        Feeder::dispense();
                        break;
                    }
                }
            }
        }
    }
}

void Scheduler::parseSchedule(const String &message) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, message);

    JsonArray scheduleArray = doc["schedules"];

    for (int i = 0; i < 2 && i < scheduleArray.size(); i++) {
        JsonObject scheduleObj = scheduleArray[i];
        _schedules[i].hour = scheduleObj["hour"];
        _schedules[i].minute = scheduleObj["minute"];
        _schedules[i].enabled = scheduleObj["enabled"];
    }

    Serial.printf("Schedule updated: %02d:%02d (%s), %02d:%02d (%s)\n",
                  _schedules[0].hour, _schedules[0].minute, _schedules[0].enabled ? "ON" : "OFF",
                  _schedules[1].hour, _schedules[1].minute, _schedules[1].enabled ? "ON" : "OFF");
}

void Scheduler::sendScheduleStatus() {
    DynamicJsonDocument doc(256);
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
}

bool Scheduler::isEnabled() {
    return _enabled;
}