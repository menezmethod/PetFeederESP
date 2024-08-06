#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

class BLEManager {
public:
    static void init();
    static void update();

private:
    static BLEServer *pServer;
    static BLECharacteristic *pCharacteristicWiFi;
    static bool deviceConnected;

    class ServerCallbacks: public BLEServerCallbacks {
        void onConnect(BLEServer* pServer);
        void onDisconnect(BLEServer* pServer);
    };

    class CharacteristicCallbacks: public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pCharacteristic);
    };
};
