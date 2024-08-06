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
    while (!_client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (_client.connect("ESP32Feeder")) {
            Serial.println("connected");
            _connected = true;
            subscribe("feeder/#");
            Scheduler::sendScheduleStatus();
            Feeder::sendStatus();
            break;
        } else {
            Serial.printf("failed, rc=%d. Trying again in 5 seconds\n", _client.state());
            delay(5000);
        }
    }
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
    String message = String((char*)payload).substring(0, length);
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