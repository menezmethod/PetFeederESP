#include "power_manager.h"
#include "../config.h"
#include <WiFi.h>
#include <esp_sleep.h>

void PowerManager::init() {
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_18, 0);
}

void PowerManager::update() {
    if (shouldEnterDeepSleep()) {
        enterDeepSleep();
    } else if (shouldEnterLightSleep()) {
        enterLightSleep();
    }
}

bool PowerManager::shouldEnterDeepSleep() {
    static unsigned long lastSleepCheck = 0;
    unsigned long now = millis();
    if (now - lastSleepCheck > DEEP_SLEEP_DURATION) {
        lastSleepCheck = now;
        return true;
    }
    return false;
}

bool PowerManager::shouldEnterLightSleep() {
    static unsigned long lastLightSleepCheck = 0;
    unsigned long now = millis();
    if (now - lastLightSleepCheck > LIGHT_SLEEP_DURATION) {
        lastLightSleepCheck = now;
        return true;
    }
    return false;
}

void PowerManager::enterDeepSleep() {
    Serial.println("Entering deep sleep mode");
    WiFi.disconnect(true);
    delay(100);
    esp_deep_sleep_start();
}

void PowerManager::enterLightSleep() {
    Serial.println("Entering light sleep mode");
    WiFi.disconnect(true);
    delay(100);
    esp_light_sleep_start();
    wakeUp();
}

void PowerManager::wakeUp() {
    Serial.println("Waking up from sleep");
    WiFi.begin();
    delay(100);
}
