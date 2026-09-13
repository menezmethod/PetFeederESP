#pragma once

#include <Arduino.h>

class OTAManager {
public:
    // Fetches OTA_VERSION_URL, compares against FIRMWARE_VERSION, and applies
    // the update if newer. Blocking (network fetch + flash write can take
    // tens of seconds) -- only ever called from an explicit MQTT command,
    // never from the main loop, same reasoning as WiFiManager::scanNetworksJson().
    // Refuses to run while Feeder::isDispensing() is true.
    static void checkForUpdate();

private:
    static void publishStatus(const char* status, const char* detail = "");
};
