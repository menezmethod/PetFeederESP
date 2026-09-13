#pragma once

#define DEVICE_NAME "ESP_FEEDER"
#define SERVICE_UUID "00FF"
#define CHAR_WIFI_UUID "FF01"

#define SERVO_PIN 23
#define SERVO_POWER_PIN 22
#define BUTTON_PIN 18

#define SERVO_MIN 500
#define SERVO_STOP 1500
#define SERVO_MAX 2500

// Self-hosted broker (TLS + auth), replacing the public broker.hivemq.com --
// anyone could previously publish a feed command to any deployed unit since
// the topic and broker were both public and unauthenticated.
#define MQTT_BROKER_URI "47.203.87.233"
#define MQTT_PORT 8883
#include "config_secrets.h"  // MQTT_USERNAME / MQTT_PASSWORD -- gitignored, see config_secrets.h.example

#define DEFAULT_SERVING_SIZE 1000
#define DEFAULT_SCHEDULES {{6, 0, true}, {17, 0, true}}

#define MQTT_TOPIC_PREFIX "pet_feeder_esp32/v1"
#define TOPIC_FEED MQTT_TOPIC_PREFIX "/commands/feed"
#define TOPIC_SERVING_SIZE MQTT_TOPIC_PREFIX "/settings/serving_size"
#define TOPIC_SCHEDULE MQTT_TOPIC_PREFIX "/settings/schedule"
#define TOPIC_GET_SCHEDULE MQTT_TOPIC_PREFIX "/requests/get_schedule"
#define TOPIC_SCHEDULING_ENABLE MQTT_TOPIC_PREFIX "/settings/scheduling_enable"
#define TOPIC_GET_STATUS MQTT_TOPIC_PREFIX "/requests/get_status"
#define TOPIC_STATUS MQTT_TOPIC_PREFIX "/status/general"
#define TOPIC_SCHEDULE_STATUS MQTT_TOPIC_PREFIX "/status/schedule"
#define TOPIC_LAST_FED MQTT_TOPIC_PREFIX "/status/last_fed"

#define MAX_CONNECTION_ATTEMPTS 20
#define CONNECTION_DELAY_MS 500

#define RECONNECT_INTERVAL_MS 5000
#define MAX_DISPENSE_DURATION_MS 5000  // hard ceiling regardless of servingSize -- servo must never run longer than this

#define WIFI_RECONNECT_INTERVAL_MS 5000
#define NTP_SYNC_TIMEOUT_MS 15000  // bounded -- an unreachable NTP server must never hang boot forever
#define BLE_REPROVISION_GRACE_MS 30000  // how long WiFi must stay down before BLE advertising reopens
#define BUTTON_DEBOUNCE_MS 50