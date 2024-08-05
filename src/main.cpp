#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>
#include "time.h"

#define DEVICE_NAME "ESP_FEEDER"
#define SERVICE_UUID        "00FF"
#define CHAR_WIFI_UUID      "FF01"

#define SERVO_PIN 23
#define SERVO_POWER_PIN 22
#define BUTTON_PIN 18

#define SERVO_MIN 500
#define SERVO_STOP 1500
#define SERVO_MAX 2500

#define MQTT_BROKER_URI "192.168.0.74"
#define MQTT_PORT 1883

enum WiFiSetupState {
    IDLE,
    SCANNING,
    WAITING_SSID,
    WAITING_PASSWORD,
    CONNECTING
};

WiFiSetupState setupState = IDLE;
bool wifi_connected = false;
bool mqtt_connected = false;
bool time_synced = false;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristicWiFi = NULL;
Preferences preferences;
Servo servo;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

String ssidReceived = "";
String passwordReceived = "";

static uint16_t serving_size = 1000;
typedef struct { uint8_t hour, minute; } schedule_t;
static schedule_t schedules[2] = {{8, 0}, {18, 0}};

void setServo(uint16_t duty) {
    Serial.printf("Setting servo to %d\n", duty);
    servo.writeMicroseconds(duty);
}

void dispense(uint16_t time) {
    Serial.printf("Dispensing for %d ms\n", time);
    digitalWrite(SERVO_POWER_PIN, HIGH);
    delay(200);  // Wait for power stabilization

    Serial.println("Setting servo to MAX");
    setServo(SERVO_MAX);
    delay(time);

    Serial.println("Setting servo to STOP");
    setServo(SERVO_STOP);
    delay(100);

    digitalWrite(SERVO_POWER_PIN, LOW);
    Serial.println("Dispense complete");
}

void saveWiFiCredentials() {
    Serial.println("Saving WiFi credentials");
    preferences.begin("wifi_creds", false);
    preferences.putString("ssid", ssidReceived);
    preferences.putString("password", passwordReceived);
    preferences.end();
    Serial.println("Credentials saved");
}

void getWiFiCredentials() {
    Serial.println("Retrieving WiFi credentials");
    preferences.begin("wifi_creds", true);
    ssidReceived = preferences.getString("ssid", "");
    passwordReceived = preferences.getString("password", "");
    preferences.end();
    Serial.println("Retrieved credentials:");
    Serial.println("SSID: " + ssidReceived);
    Serial.println("Password: " + passwordReceived);
}

void scanNetworks() {
    Serial.println("Scanning WiFi networks...");
    int n = WiFi.scanNetworks();
    String networks = "Available networks:\n";
    for (int i = 0; i < n; ++i) {
        networks += String(i + 1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + "dBm)\n";
    }
    Serial.println(networks);
    pCharacteristicWiFi->setValue(networks.c_str());
    pCharacteristicWiFi->notify();
}

void connectToWiFi() {
    if (ssidReceived.length() > 0 && passwordReceived.length() > 0) {
        Serial.println("Attempting to connect to WiFi");
        Serial.printf("SSID: %s\n", ssidReceived.c_str());
        WiFi.disconnect();  // Disconnect from any previous WiFi
        WiFi.begin(ssidReceived.c_str(), passwordReceived.c_str());

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi connected");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            wifi_connected = true;
            String connectedMsg = "Connected to WiFi. IP: " + WiFi.localIP().toString();
            pCharacteristicWiFi->setValue(connectedMsg.c_str());
            pCharacteristicWiFi->notify();
        } else {
            Serial.println("\nFailed to connect to WiFi");
            wifi_connected = false;
            pCharacteristicWiFi->setValue("Failed to connect to WiFi");
            pCharacteristicWiFi->notify();
        }
    } else {
        Serial.println("No WiFi credentials stored");
        pCharacteristicWiFi->setValue("No WiFi credentials stored");
        pCharacteristicWiFi->notify();
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
    Serial.println();
    time_synced = true;
    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
    tzset();
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.println(&timeinfo, "Time synchronized: %A, %B %d %Y %H:%M:%S");
    } else {
        Serial.println("Failed to obtain time");
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = String((char*)payload).substring(0, length);
    Serial.println("Message arrived [" + String(topic) + "] " + message);

    if (strcmp(topic, "feeder/feed") == 0) {
        Serial.println("Dispensing food via MQTT command");
        dispense(serving_size);
    } else if (strcmp(topic, "feeder/serving_size") == 0) {
        serving_size = message.toInt();
        Serial.printf("Serving size updated to %d ms\n", serving_size);
    } else if (strcmp(topic, "feeder/schedule") == 0) {
        int hour1, min1, hour2, min2;
        if (sscanf(message.c_str(), "%d:%d,%d:%d", &hour1, &min1, &hour2, &min2) == 4) {
            schedules[0] = {(uint8_t)hour1, (uint8_t)min1};
            schedules[1] = {(uint8_t)hour2, (uint8_t)min2};
            Serial.printf("Schedule updated: %02d:%02d, %02d:%02d\n",
                          schedules[0].hour, schedules[0].minute,
                          schedules[1].hour, schedules[1].minute);
        } else {
            Serial.println("Invalid schedule format. Use HH:MM,HH:MM");
        }
    }
}

void reconnectMQTT() {
    int attempts = 0;
    while (!mqttClient.connected() && wifi_connected && attempts < 3) {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect("ESP32Feeder")) {
            Serial.println("connected");
            mqtt_connected = true;
            mqttClient.subscribe("feeder/#");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
            attempts++;
        }
    }
    if (!mqtt_connected) {
        Serial.println("Failed to connect to MQTT after 3 attempts");
    }
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            String strValue = String(value.c_str());
            Serial.println("Received: " + strValue);

            if (strValue == "SCAN") {
                setupState = SCANNING;
            } else if (strValue.startsWith("ssid:")) {
                ssidReceived = strValue.substring(5);
                setupState = WAITING_PASSWORD;
                pCharacteristicWiFi->setValue("Received SSID. Please send password as 'pass:<your_password>'");
                pCharacteristicWiFi->notify();
            } else if (strValue.startsWith("pass:")) {
                passwordReceived = strValue.substring(5);
                setupState = CONNECTING;
                saveWiFiCredentials();
                pCharacteristicWiFi->setValue("Credentials received. Attempting to connect...");
                pCharacteristicWiFi->notify();
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
    } else {
        Serial.println("Servo was already attached");
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
            scanNetworks();
            setupState = WAITING_SSID;
            pCharacteristicWiFi->setValue("Scan complete. Please send SSID as 'ssid:<your_ssid>'");
            pCharacteristicWiFi->notify();
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

        static bool timePrinted = false;
        static unsigned long lastCheck = 0;
        unsigned long now = millis();
        if (now - lastCheck > 10000) {
            lastCheck = now;
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                if (!timePrinted) {
                    Serial.printf("Current time: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                    timePrinted = true;
                }
                for (int i = 0; i < 2; i++) {
                    if (timeinfo.tm_hour == schedules[i].hour &&
                        timeinfo.tm_min == schedules[i].minute &&
                        timeinfo.tm_sec < 10) { // Allow a 10-second window for feeding
                        Serial.println("Scheduled feeding time");
                        dispense(serving_size);
                        break;
                    }
                }
            } else {
                Serial.println("Failed to obtain time");
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
