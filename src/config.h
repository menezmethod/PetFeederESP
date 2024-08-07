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

#define MQTT_BROKER_URI "192.168.0.221"
#define MQTT_PORT 1883

#define DEEP_SLEEP_DURATION 10 * 60 * 1000000  // 10 minutes in microseconds
#define LIGHT_SLEEP_DURATION 30 * 1000000  // 30 seconds in microseconds

#define DEFAULT_SERVING_SIZE 1000
#define DEFAULT_SCHEDULES {{6, 0, true}, {17, 0, true}}

#define TOPIC_FEED "feeder/feed"
#define TOPIC_SERVING_SIZE "feeder/serving_size"
#define TOPIC_SCHEDULE "feeder/schedule"
#define TOPIC_GET_SCHEDULE "feeder/get_schedule"
#define TOPIC_SCHEDULING_ENABLE "feeder/scheduling_enable"
#define TOPIC_GET_STATUS "feeder/get_status"
#define TOPIC_STATUS "feeder/status"
#define TOPIC_SCHEDULE_STATUS "feeder/schedule_status"

#define MAX_CONNECTION_ATTEMPTS 20
#define CONNECTION_DELAY_MS 500
