#include "wifi_manager.h"
#include "../config.h"

String WiFiManager::_ssid = "";
String WiFiManager::_password = "";
bool WiFiManager::_connected = false;
bool WiFiManager::_connecting = false;
unsigned long WiFiManager::_connectStartTime = 0;
unsigned long WiFiManager::_lastAttempt = 0;
Preferences WiFiManager::_preferences;

void WiFiManager::init() {
    loadCredentials();
    WiFi.onEvent(WiFiEvent);
    if (_ssid.length() > 0 && _password.length() > 0) {
        connectToWiFi();
    }
}

void WiFiManager::update() {
    if (_connected) return;
    if (_ssid.length() == 0 || _password.length() == 0) return;

    unsigned long now = millis();
    if (_connecting) {
        // WiFiEvent flips _connected on success -- nothing to do here but
        // notice a timeout. No blocking wait, unlike the original version.
        if (now - _connectStartTime > (unsigned long)MAX_CONNECTION_ATTEMPTS * CONNECTION_DELAY_MS) {
            Serial.println("WiFi connection attempt timed out, will retry");
            _connecting = false;
            _lastAttempt = now;
        }
        return;
    }
    if (now - _lastAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
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
    // Non-blocking: just kicks off the attempt. WiFiEvent() below reports
    // success asynchronously; update() reports a timeout if it never comes.
    // The old version blocked here for up to MAX_CONNECTION_ATTEMPTS *
    // CONNECTION_DELAY_MS (10s), during which Feeder::update() -- and every
    // other module -- never ran, same failure class as the MQTT reconnect bug.
    Serial.printf("Connecting to WiFi: %s\n", _ssid.c_str());
    WiFi.begin(_ssid.c_str(), _password.c_str());
    _connecting = true;
    _connectStartTime = millis();
    _lastAttempt = _connectStartTime;
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
            _connecting = false;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi lost connection");
            _connected = false;
            _connecting = false;
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("WiFi IP obtained: ");
            Serial.println(WiFi.localIP());
            _connected = true;
            _connecting = false;
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
