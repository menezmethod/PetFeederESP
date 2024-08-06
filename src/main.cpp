#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Preferences.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ctime>

#define DEVICE_NAME "ESP_FEEDER"
#define SERVICE_UUID        "00FF"
#define CHAR_WIFI_UUID      "FF01"

#define SERVO_PIN 23
#define SERVO_POWER_PIN 22
#define BUTTON_PIN 18

#define SERVO_MIN 500
#define SERVO_STOP 1500
#define SERVO_MAX 2500

#define MQTT_BROKER_URI "192.168.0.221"
#define MQTT_PORT 1883

enum WiFiSetupState { IDLE, SCANNING, WAITING_SSID, WAITING_PASSWORD, CONNECTING };

WiFiSetupState setupState = IDLE;
bool wifi_connected = false;
bool mqtt_connected = false;
bool time_synced = false;
bool schedulingEnabled = true;  // Global scheduling is on by default

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristicWiFi = NULL;
Preferences preferences;
Servo servo;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

String ssidReceived = "";
String passwordReceived = "";

static uint16_t serving_size = 1000;

typedef struct {
    int hour, minute;
    bool enabled;
} schedule_t;

static schedule_t schedules[2] = {{8, 0, true}, {18, 0, true}};

void setServo(uint16_t duty) {
    Serial.printf("Setting servo to %d\n", duty);
    servo.writeMicroseconds(duty);
}

void dispense(uint16_t time) {
    Serial.printf("Dispensing for %d ms\n", time);
    digitalWrite(SERVO_POWER_PIN, HIGH);
    delay(200);  // Wait for power stabilization
    setServo(SERVO_MAX);
    delay(time);
    setServo(SERVO_STOP);
    delay(100);
    digitalWrite(SERVO_POWER_PIN, LOW);
    Serial.println("Dispense complete");
}

void saveWiFiCredentials() {
    preferences.begin("wifi_creds", false);
    preferences.putString("ssid", ssidReceived);
    preferences.putString("password", passwordReceived);
    preferences.end();
    Serial.println("WiFi credentials saved");
}

void getWiFiCredentials() {
    preferences.begin("wifi_creds", true);
    ssidReceived = preferences.getString("ssid", "");
    passwordReceived = preferences.getString("password", "");
    preferences.end();
    Serial.println("Retrieved WiFi credentials");
}

void connectToWiFi() {
    if (ssidReceived.length() > 0 && passwordReceived.length() > 0) {
        Serial.printf("Connecting to WiFi: %s\n", ssidReceived.c_str());
        WiFi.begin(ssidReceived.c_str(), passwordReceived.c_str());
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("\nWiFi connected. IP: %s\n", WiFi.localIP().toString().c_str());
            wifi_connected = true;
        } else {
            Serial.println("\nFailed to connect to WiFi");
            wifi_connected = false;
        }
    } else {
        Serial.println("No WiFi credentials stored");
    }
}

void syncTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for time sync");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    time_synced = true;
    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
    tzset();
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.printf("Time synchronized: %s", asctime(&timeinfo));
    } else {
        Serial.println("Failed to obtain time");
    }
}

void sendStatus() {
    DynamicJsonDocument doc(256);
    doc["schedulingEnabled"] = schedulingEnabled;
    doc["servingSize"] = serving_size;
    String jsonString;
    serializeJson(doc, jsonString);
    mqttClient.publish("feeder/status", jsonString.c_str());
}

void sendScheduleStatus() {
    DynamicJsonDocument doc(256);
    doc["enabled"] = schedulingEnabled;
    JsonArray scheduleArray = doc.createNestedArray("schedules");
    for (int i = 0; i < 2; i++) {
        JsonObject scheduleObj = scheduleArray.createNestedObject();
        scheduleObj["hour"] = schedules[i].hour;
        scheduleObj["minute"] = schedules[i].minute;
        scheduleObj["enabled"] = schedules[i].enabled;
    }
    String jsonString;
    serializeJson(doc, jsonString);
    mqttClient.publish("feeder/schedule_status", jsonString.c_str());
}

void parseSchedule(const String& message) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, message);

    schedulingEnabled = doc["enabled"];
    JsonArray scheduleArray = doc["schedules"];

    for (int i = 0; i < 2 && i < scheduleArray.size(); i++) {
        JsonObject scheduleObj = scheduleArray[i];
        schedules[i].hour = scheduleObj["hour"];
        schedules[i].minute = scheduleObj["minute"];
        schedules[i].enabled = scheduleObj["enabled"];
    }

    Serial.printf("Schedule updated: %02d:%02d (%s), %02d:%02d (%s)\n",
                  schedules[0].hour, schedules[0].minute, schedules[0].enabled ? "ON" : "OFF",
                  schedules[1].hour, schedules[1].minute, schedules[1].enabled ? "ON" : "OFF");
    Serial.printf("Global scheduling: %s\n", schedulingEnabled ? "ON" : "OFF");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = String((char*)payload).substring(0, length);
    Serial.printf("Message arrived [%s] %s\n", topic, message.c_str());

    if (strcmp(topic, "feeder/feed") == 0) {
        dispense(serving_size);
    } else if (strcmp(topic, "feeder/serving_size") == 0) {
        serving_size = message.toInt();
        Serial.printf("Serving size updated to %d ms\n", serving_size);
        sendStatus(); // Send updated status
    } else if (strcmp(topic, "feeder/schedule") == 0) {
        parseSchedule(message);
        sendScheduleStatus();
    } else if (strcmp(topic, "feeder/get_schedule") == 0) {
        sendScheduleStatus();
    } else if (strcmp(topic, "feeder/scheduling_enable") == 0) {
        schedulingEnabled = (message == "1" || message.equalsIgnoreCase("true") || message.equalsIgnoreCase("on"));
        Serial.printf("Global scheduling %s\n", schedulingEnabled ? "enabled" : "disabled");
        sendScheduleStatus();
        sendStatus(); // Send updated status
    } else if (strcmp(topic, "feeder/get_status") == 0) {
        sendStatus();
    }
}

void reconnectMQTT() {
    while (!mqttClient.connected() && wifi_connected) {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect("ESP32Feeder")) {
            Serial.println("connected");
            mqtt_connected = true;
            mqttClient.subscribe("feeder/#");
            sendScheduleStatus();
            sendStatus(); // Send status on reconnect
            break;
        } else {
            Serial.printf("failed, rc=%d. Trying again in 5 seconds\n", mqttClient.state());
            delay(5000);
        }
    }
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            String strValue = String(value.c_str());
            Serial.printf("Received via BLE: %s\n", strValue.c_str());

            if (strValue == "SCAN") {
                setupState = SCANNING;
            } else if (strValue.startsWith("ssid:")) {
                ssidReceived = strValue.substring(5);
                setupState = WAITING_PASSWORD;
            } else if (strValue.startsWith("pass:")) {
                passwordReceived = strValue.substring(5);
                setupState = CONNECTING;
                saveWiFiCredentials();
            }
        }
    }
};

void setupBLE() {
    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristicWiFi = pService->createCharacteristic(
                            CHAR_WIFI_UUID,
                            BLECharacteristic::PROPERTY_READ |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY
                          );
    pCharacteristicWiFi->setCallbacks(new MyCallbacks());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("BLE Advertising started");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("ESP32 Pet Feeder starting up...");

    pinMode(SERVO_POWER_PIN, OUTPUT);
    digitalWrite(SERVO_POWER_PIN, LOW);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    if (!servo.attached()) {
        servo.attach(SERVO_PIN, SERVO_MIN, SERVO_MAX);
        Serial.println("Servo attached");
    }
    setServo(SERVO_STOP);

    setupBLE();
    getWiFiCredentials();
    if (ssidReceived.length() > 0 && passwordReceived.length() > 0) {
        connectToWiFi();
    }

    if (wifi_connected) {
        syncTime();
    }

    mqttClient.setServer(MQTT_BROKER_URI, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
}

void loop() {
    switch (setupState) {
        case SCANNING:
            // Implement WiFi scanning logic here if needed
            setupState = WAITING_SSID;
            break;
        case CONNECTING:
            connectToWiFi();
            setupState = IDLE;
            break;
        default:
            break;
    }

    if (wifi_connected) {
        if (!mqttClient.connected()) {
            reconnectMQTT();
        }
        if (mqtt_connected) {
            mqttClient.loop();
        }

        if (!time_synced && WiFi.status() == WL_CONNECTED) {
            syncTime();
        }

        static unsigned long lastCheck = 0;
        unsigned long now = millis();
        if (now - lastCheck > 10000) {
            lastCheck = now;
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                if (schedulingEnabled) {
                    for (int i = 0; i < 2; i++) {
                        if (schedules[i].enabled &&
                            timeinfo.tm_hour == schedules[i].hour &&
                            timeinfo.tm_min == schedules[i].minute &&
                            timeinfo.tm_sec < 10) { // Allow a 10-second window for feeding
                            Serial.println("Scheduled feeding time");
                            dispense(serving_size);
                            break;
                        }
                    }
                }
            }
        }

        if (digitalRead(BUTTON_PIN) == LOW) {
            Serial.println("Button pressed");
            dispense(serving_size);
            delay(500); // Debounce
        }
    } else {
        delay(1000);
    }
}
