#include "ble_manager.h"
#include "../config.h"
#include "wifi_manager.h"
#include "feeder.h"

BLEServer *BLEManager::pServer = nullptr;
BLECharacteristic *BLEManager::pCharacteristicWiFi = nullptr;
BLECharacteristic *BLEManager::pCharacteristicWiFiScan = nullptr;
bool BLEManager::deviceConnected = false;
bool BLEManager::advertisingEnabled = true;
bool BLEManager::scanPending = false;

void BLEManager::init() {
    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristicWiFi = pService->createCharacteristic(
                            CHAR_WIFI_UUID,
                            BLECharacteristic::PROPERTY_READ |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY
                          );
    pCharacteristicWiFi->setCallbacks(new CharacteristicCallbacks());
    pCharacteristicWiFi->addDescriptor(new BLE2902());

    // Read-only: populated with nearby networks shortly after connect (see
    // update()) so the app can show a picker instead of asking the user to
    // type an SSID blind.
    pCharacteristicWiFiScan = pService->createCharacteristic(
                            CHAR_WIFI_SCAN_UUID,
                            BLECharacteristic::PROPERTY_READ |
                            BLECharacteristic::PROPERTY_NOTIFY
                          );
    pCharacteristicWiFiScan->setValue("{\"networks\":[]}");
    pCharacteristicWiFiScan->addDescriptor(new BLE2902());

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("BLE Advertising started");
}

void BLEManager::update() {
    // WiFi.scanNetworks() blocks for 1-3s -- same failure class as the MQTT
    // and WiFi-connect blocking bugs fixed earlier: running it unconditionally
    // here would stall loop() and let a dispense overrun its safety ceiling
    // (Feeder::update() wouldn't run for the scan's duration). Deferred, not
    // dropped: scanPending stays true and this retries next loop iteration.
    if (deviceConnected && scanPending && !Feeder::isDispensing()) {
        scanPending = false;
        String json = WiFiManager::scanNetworksJson();
        pCharacteristicWiFiScan->setValue(json.c_str());
        pCharacteristicWiFiScan->notify();
    }
}

void BLEManager::setAdvertisingEnabled(bool enabled) {
    if (enabled == advertisingEnabled) return;
    advertisingEnabled = enabled;
    if (enabled) {
        BLEDevice::startAdvertising();
        Serial.println("BLE advertising resumed (WiFi down -- re-provisioning available)");
    } else {
        BLEDevice::getAdvertising()->stop();
        Serial.println("BLE advertising stopped (WiFi provisioned)");
    }
}

void BLEManager::ServerCallbacks::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    scanPending = true;  // actual scan runs from update(), not this callback
    Serial.println("BLE device connected");
}

void BLEManager::ServerCallbacks::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    if (advertisingEnabled) {
        BLEDevice::startAdvertising();
        Serial.println("BLE device disconnected, advertising restarted");
    }
}

void BLEManager::CharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
        String strValue = String(value.c_str());
        Serial.printf("Received via BLE: %s\n", strValue.c_str());

        if (strValue.startsWith("ssid:")) {
            String ssid = strValue.substring(5);
            Serial.printf("Received SSID: %s\n", ssid.c_str());
            WiFiManager::setCredentials(ssid, WiFiManager::getPassword());
        } else if (strValue.startsWith("pass:")) {
            String password = strValue.substring(5);
            Serial.printf("Received Password: %s\n", password.c_str());
            WiFiManager::setCredentials(WiFiManager::getSSID(), password);
            WiFiManager::connectToWiFi();
        }
    }
}
