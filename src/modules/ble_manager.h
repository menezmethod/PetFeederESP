#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

class BLEManager {
public:
    static void init();
    static void update();
    static void setAdvertisingEnabled(bool enabled);

private:
    static BLEServer *pServer;
    static BLECharacteristic *pCharacteristicWiFi;
    static BLECharacteristic *pCharacteristicWiFiScan;
    static bool deviceConnected;
    static bool advertisingEnabled;
    static bool scanPending;

    class ServerCallbacks: public BLEServerCallbacks {
        void onConnect(BLEServer* pServer);
        void onDisconnect(BLEServer* pServer);
    };

    class CharacteristicCallbacks: public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pCharacteristic);
    };
};
