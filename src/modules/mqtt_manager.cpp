#include "mqtt_manager.h"
#include "../config.h"
#include "scheduler.h"
#include "feeder.h"

WiFiClient MQTTManager::_wifiClient;
PubSubClient MQTTManager::_client(MQTTManager::_wifiClient);
bool MQTTManager::_connected = false;

void MQTTManager::init() {
    _client.setServer(MQTT_BROKER_URI, MQTT_PORT);
    _client.setCallback(callback);
}

void MQTTManager::update() {
    if (!_client.connected()) {
        reconnect();
    }
    _client.loop();
}

bool MQTTManager::isConnected() {
    return _connected;
}

void MQTTManager::publish(const char* topic, const char* payload) {
    _client.publish(topic, payload);
}

void MQTTManager::subscribe(const char* topic) {
    _client.subscribe(topic);
}

void MQTTManager::disconnect() {
    _client.disconnect();
    _connected = false;
}

void MQTTManager::reconnect() {
    // Non-blocking: a blocking retry loop here stalls the whole loop() while a
    // dispense may be in progress, so Feeder::update() never runs to stop the
    // servo. One attempt per call, gated by RECONNECT_INTERVAL_MS.
    static unsigned long lastAttempt = 0;
    unsigned long now = millis();
    if (now - lastAttempt < RECONNECT_INTERVAL_MS) {
        return;
    }
    lastAttempt = now;

    Serial.print("Attempting MQTT connection...");
    if (_client.connect("ESP32Feeder")) {
        Serial.println("connected");
        _connected = true;
        subscribe(MQTT_TOPIC_PREFIX "/#");
        Scheduler::sendScheduleStatus();
        Feeder::sendStatus();
    } else {
        Serial.printf("failed, rc=%d. Retrying in %lus\n", _client.state(), RECONNECT_INTERVAL_MS / 1000);
        _connected = false;
    }
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
    // PubSubClient payloads are not null-terminated; build the String from
    // the given length rather than scanning the buffer for a terminator.
    String message;
    message.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.printf("Message arrived [%s] %s\n", topic, message.c_str());

    if (strcmp(topic, TOPIC_FEED) == 0) {
        Feeder::dispense();
    } else if (strcmp(topic, TOPIC_SERVING_SIZE) == 0) {
        Feeder::setServingSize(message.toInt());
        Feeder::sendStatus();
    } else if (strcmp(topic, TOPIC_SCHEDULE) == 0) {
        Scheduler::parseSchedule(message);
        Scheduler::sendScheduleStatus();
    } else if (strcmp(topic, TOPIC_GET_SCHEDULE) == 0) {
        Scheduler::sendScheduleStatus();
    } else if (strcmp(topic, TOPIC_SCHEDULING_ENABLE) == 0) {
        bool enabled = (message == "1" || message.equalsIgnoreCase("true") || message.equalsIgnoreCase("on"));
        Scheduler::setEnabled(enabled);
        Scheduler::sendScheduleStatus();
        Feeder::sendStatus();
    } else if (strcmp(topic, TOPIC_GET_STATUS) == 0) {
        Feeder::sendStatus();
        Scheduler::sendScheduleStatus();
    }
}