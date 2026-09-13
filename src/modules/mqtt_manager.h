#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

class MQTTManager {
public:
    static void init();

    static void update();

    static bool isConnected();

    static void publish(const char *topic, const char *payload, bool retained = false);

    static void subscribe(const char *topic);

    static void disconnect();

private:
    static void reconnect();

    static void callback(char *topic, byte *payload, unsigned int length);

    static WiFiClientSecure _wifiClient;
    static PubSubClient _client;
    static bool _connected;
    static String _clientId;
};
