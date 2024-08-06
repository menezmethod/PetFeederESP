#include "wifi_manager.h"
#include "../config.h"

String WiFiManager::_ssid = "";
String WiFiManager::_password = "";
bool WiFiManager::_connected = false;
Preferences WiFiManager::_preferences;

void WiFiManager::init() {
    loadCredentials();
    WiFi.onEvent(WiFiEvent);
    if (_ssid.length() > 0 && _password.length() > 0) {
        connectToWiFi();
    }
}

void WiFiManager::update() {
    if (!_connected && _ssid.length() > 0 && _password.length() > 0) {
        connectToWiFi();
    }
}

bool WiFiManager::isConnected() {
    return _connected;
}

void WiFiManager::setCredentials(const String &ssid, const String &password) {
    _ssid = ssid;
    _password = password;
    saveCredentials();
}

String WiFiManager::getIP() {
    return WiFi.localIP().toString();
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true);
    _connected = false;
}

void WiFiManager::connectToWiFi() {
    Serial.printf("Connecting to WiFi: %s\n", _ssid.c_str());
    WiFi.begin(_ssid.c_str(), _password.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < MAX_CONNECTION_ATTEMPTS) {
        delay(CONNECTION_DELAY_MS);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi connected. IP: %s\n", getIP().c_str());
        _connected = true;
    } else {
        Serial.println("\nFailed to connect to WiFi");
        _connected = false;
    }
}

void WiFiManager::saveCredentials() {
    _preferences.begin("wifi_creds", false);
    _preferences.putString("ssid", _ssid);
    _preferences.putString("password", _password);
    _preferences.end();
    Serial.println("WiFi credentials saved");
}

void WiFiManager::loadCredentials() {
    _preferences.begin("wifi_creds", true);
    _ssid = _preferences.getString("ssid", "");
    _password = _preferences.getString("password", "");
    _preferences.end();
    Serial.println("Retrieved WiFi credentials");
}

void WiFiManager::WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("WiFi connected");
            _connected = true;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi lost connection");
            _connected = false;
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("WiFi IP obtained: ");
            Serial.println(WiFi.localIP());
            _connected = true;
            break;
        default:
            break;
    }
}

String WiFiManager::getSSID() {
    return _ssid;
}

String WiFiManager::getPassword() {
    return _password;
}
