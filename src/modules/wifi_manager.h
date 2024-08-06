#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

class WiFiManager {
public:
    static void init();

    static void update();

    static bool isConnected();

    static void setCredentials(const String &ssid, const String &password);

    static String getIP();

    static void disconnect();

    static void connectToWiFi(); // Moved to public
    static String getSSID();

    static String getPassword();

private:
    static void saveCredentials();

    static void loadCredentials();

    static void WiFiEvent(WiFiEvent_t event);

    static String _ssid;
    static String _password;
    static bool _connected;
    static Preferences _preferences;
};
