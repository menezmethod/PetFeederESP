#pragma once

#define DEVICE_NAME "ESP_FEEDER"
#define SERVICE_UUID "00FF"
#define CHAR_WIFI_UUID "FF01"
#define CHAR_WIFI_SCAN_UUID "FF02"  // read-only: JSON array of nearby networks, populated on BLE connect

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

// Days bitmask: bit 0 = Sunday ... bit 6 = Saturday, matching struct tm's
// tm_wday directly (no reindexing needed at the compare site). 0x7F = every day.
#define SCHEDULE_ALL_DAYS 0x7F
#define MAX_SCHEDULES 6
#define DEFAULT_SCHEDULE_COUNT 2
#define DEFAULT_SCHEDULES {{6, 0, true, SCHEDULE_ALL_DAYS}, {17, 0, true, SCHEDULE_ALL_DAYS}, {0, 0, false, SCHEDULE_ALL_DAYS}, {0, 0, false, SCHEDULE_ALL_DAYS}, {0, 0, false, SCHEDULE_ALL_DAYS}, {0, 0, false, SCHEDULE_ALL_DAYS}}

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
#define TOPIC_OTA_CHECK MQTT_TOPIC_PREFIX "/commands/ota_check"
#define TOPIC_OTA_STATUS MQTT_TOPIC_PREFIX "/status/ota"

// Bump this with every release. Compared against OTA_VERSION_URL's
// "version" field to decide whether an update is available.
#define FIRMWARE_VERSION "1.1.0"
#define OTA_VERSION_URL "https://github.com/menezmethod/PetFeederESP/releases/latest/download/version.json"

#define MAX_CONNECTION_ATTEMPTS 20
#define CONNECTION_DELAY_MS 500

#define RECONNECT_INTERVAL_MS 5000
#define MIN_DISPENSE_DURATION_MS 100   // floor -- a 0ms "dispense" is a silent no-op, not a rejected request
#define MAX_DISPENSE_DURATION_MS 5000  // hard ceiling regardless of servingSize -- servo must never run longer than this

#define WIFI_RECONNECT_INTERVAL_MS 5000
#define NTP_SYNC_TIMEOUT_MS 15000  // bounded -- an unreachable NTP server must never hang boot forever
#define BLE_REPROVISION_GRACE_MS 30000  // how long WiFi must stay down before BLE advertising reopens
#define BUTTON_DEBOUNCE_MS 50