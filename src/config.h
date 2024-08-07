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

#define MQTT_BROKER_URI "broker.hivemq.com"
#define MQTT_PORT 1883

#define DEEP_SLEEP_DURATION 10 * 60 * 1000000  // 10 minutes in microseconds
#define LIGHT_SLEEP_DURATION 30 * 1000000  // 30 seconds in microseconds

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

#define MAX_CONNECTION_ATTEMPTS 20
#define CONNECTION_DELAY_MS 500