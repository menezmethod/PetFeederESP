#include "config.h"
#include "modules/wifi_manager.h"
#include "modules/ble_manager.h"
#include "modules/mqtt_manager.h"
#include "modules/feeder.h"
#include "modules/scheduler.h"

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("Pet Feeder starting up...");

    Feeder::init();
    BLEManager::init();
    WiFiManager::init();
    // MQTTManager::init() and Scheduler::init() used to be gated on
    // WiFiManager::isConnected() at this exact instant -- since connecting is
    // now asynchronous (see wifi_manager.cpp), that gate would almost always
    // be false here and these would never run at all, not just later.
    // Both are now safe to call unconditionally: MQTTManager::init() only
    // configures the client (no network needed yet), and Scheduler::init()'s
    // NTP sync is now bounded (see time_utils.cpp) instead of hanging forever.
    MQTTManager::init();
    Scheduler::init();
}

void loop() {
    BLEManager::update();
    WiFiManager::update();

    // BLE provisioning closes once WiFi is up (removes the standing
    // unauthenticated write-your-own-credentials surface) and reopens if
    // WiFi stays down past a grace period, so re-provisioning is still
    // possible without leaving the characteristic open permanently.
    static bool bleAdvertising = true;
    static unsigned long disconnectedSince = 0;
    if (WiFiManager::isConnected()) {
        if (bleAdvertising) {
            BLEManager::setAdvertisingEnabled(false);
            bleAdvertising = false;
        }
        disconnectedSince = 0;
    } else {
        if (disconnectedSince == 0) disconnectedSince = millis();
        if (!bleAdvertising && millis() - disconnectedSince > BLE_REPROVISION_GRACE_MS) {
            BLEManager::setAdvertisingEnabled(true);
            bleAdvertising = true;
        }
    }

    if (WiFiManager::isConnected()) {
        MQTTManager::update();
        Scheduler::update();
    }

    Feeder::update();
}
