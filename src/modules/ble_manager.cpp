#include "ble_manager.h"
#include "../config.h"
#include "wifi_manager.h"

BLEServer *BLEManager::pServer = nullptr;
BLECharacteristic *BLEManager::pCharacteristicWiFi = nullptr;
bool BLEManager::deviceConnected = false;

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
    if (deviceConnected) {
        // Update characteristic value, if needed
    }
}

void BLEManager::ServerCallbacks::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE device connected");
}

void BLEManager::ServerCallbacks::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    BLEDevice::startAdvertising();
    Serial.println("BLE device disconnected, advertising restarted");
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
