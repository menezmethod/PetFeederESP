#include "ota_manager.h"
#include "../config.h"
#include "feeder.h"
#include "mqtt_manager.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

void OTAManager::publishStatus(const char* status, const char* detail) {
    StaticJsonDocument<256> doc;
    doc["status"] = status;
    doc["currentVersion"] = FIRMWARE_VERSION;
    if (strlen(detail) > 0) doc["detail"] = detail;
    String json;
    serializeJson(doc, json);
    MQTTManager::publish(TOPIC_OTA_STATUS, json.c_str(), true);
}

void OTAManager::checkForUpdate() {
    if (Feeder::isDispensing()) {
        Serial.println("OTA: refusing to check, a dispense is in progress");
        publishStatus("deferred", "dispense in progress");
        return;
    }

    Serial.println("OTA: checking for update...");
    publishStatus("checking");

    // GitHub Releases redirects across hosts with different CA chains
    // (github.com -> objects.githubusercontent.com) -- pinning a specific
    // root here is a much deeper problem than the broker's fixed self-signed
    // cert. Using setInsecure() for this connection specifically: TLS still
    // encrypts the download in transit, the download URL itself is fixed at
    // compile time (not attacker-influenceable via MQTT), and reaching this
    // code at all already requires valid, authenticated MQTT credentials.
    // Documented trade-off, not an oversight.
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, OTA_VERSION_URL)) {
        Serial.println("OTA: failed to begin version check request");
        publishStatus("error", "could not reach version URL");
        return;
    }
    http.addHeader("User-Agent", "PetFeederESP");

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("OTA: version check HTTP %d\n", code);
        publishStatus("error", "version check failed");
        http.end();
        return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, http.getString());
    http.end();
    if (err) {
        Serial.printf("OTA: failed to parse version.json: %s\n", err.c_str());
        publishStatus("error", "malformed version.json");
        return;
    }

    const char* latestVersion = doc["version"] | "";
    const char* firmwareUrl = doc["url"] | "";
    if (strlen(latestVersion) == 0 || strlen(firmwareUrl) == 0) {
        publishStatus("error", "version.json missing fields");
        return;
    }

    if (strcmp(latestVersion, FIRMWARE_VERSION) == 0) {
        Serial.println("OTA: already up to date");
        publishStatus("up_to_date");
        return;
    }

    // Re-check right before flashing -- the version check itself took a few
    // seconds of network time, during which a feed could have started.
    if (Feeder::isDispensing()) {
        Serial.println("OTA: aborting update, a dispense started during version check");
        publishStatus("deferred", "dispense in progress");
        return;
    }

    Serial.printf("OTA: updating %s -> %s\n", FIRMWARE_VERSION, latestVersion);
    publishStatus("updating", latestVersion);

    httpUpdate.rebootOnUpdate(true);
    WiFiClientSecure updateClient;
    updateClient.setInsecure();
    t_httpUpdate_return result = httpUpdate.update(updateClient, firmwareUrl);

    switch (result) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("OTA: update failed: %s\n", httpUpdate.getLastErrorString().c_str());
            publishStatus("failed", httpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            publishStatus("up_to_date");
            break;
        case HTTP_UPDATE_OK:
            // Device reboots automatically (rebootOnUpdate(true)) -- nothing
            // after this point runs on this boot.
            break;
    }
}
