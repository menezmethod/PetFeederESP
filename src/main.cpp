#include "modules/wifi_manager.h"
#include "modules/ble_manager.h"
#include "modules/mqtt_manager.h"
#include "modules/feeder.h"
#include "modules/scheduler.h"
#include "modules/power_manager.h"

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("Pet Feeder starting up...");

    PowerManager::init();
    Feeder::init();
    BLEManager::init();
    WiFiManager::init();

    if (WiFiManager::isConnected()) {
        MQTTManager::init();
        Scheduler::init();
    }

    PowerManager::wakeUp();
}

void loop() {
    BLEManager::update();
    WiFiManager::update();

    if (WiFiManager::isConnected()) {
        MQTTManager::update();
        Scheduler::update();
    }

    Feeder::update();
    PowerManager::update();
}
