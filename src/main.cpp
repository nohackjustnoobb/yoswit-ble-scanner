#include "credentials.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <map>
#include <string>

#define SCAN_DURATION_SECONDS 5
#define MAX_CONNECTION_ATTEMPTS 30
#define CONNECTION_RETRY_TIMEOUT_MS 1000
#define LED_PIN 8

// Global variable to store ManufacturerData of size 9, keyed by MAC address
std::map<std::string, std::string> devices;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

BLEScan *pBLEScan;
bool scanning = false;

void connectWiFi() {
  int attempts = 0;
  Serial.print("Connecting to WiFi (SSID: ");
  Serial.print(WIFI_SSID);
  Serial.print(")");

  while (WiFi.status() != WL_CONNECTED && attempts < MAX_CONNECTION_ATTEMPTS) {
    if (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) == WL_CONNECTED)
      break;

    delay(CONNECTION_RETRY_TIMEOUT_MS);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed");
  }
}

void publishDeviceData(const std::string &mac, const std::string &data) {
  // Create message: MAC address followed by hex data
  String message = String(mac.c_str());

  for (unsigned char c : data) {
    char hex[3];
    sprintf(hex, "%02X", (unsigned char)c);
    message += hex;
  }

  mqttClient.publish(MQTT_TOPIC, message.c_str());
  Serial.print("Published to MQTT: ");
  Serial.println(message);
}

void publishAllDevices() {
  Serial.println("Publishing all devices to MQTT...");

  for (const auto &device : devices)
    publishDeviceData(device.first, device.second);

  Serial.print("Published ");
  Serial.print(devices.size());
  Serial.println(" devices");
}

void connectMQTT() {
  int attempts = 0;
  Serial.print("Connecting to MQTT (Broker: ");
  Serial.print(MQTT_BROKER);
  Serial.print(":");
  Serial.print(MQTT_PORT);
  Serial.print(")");

  while (!mqttClient.connected() && attempts < MAX_CONNECTION_ATTEMPTS) {
    if (mqttClient.connect(MQTT_CLIENT_ID))
      break;

    delay(CONNECTION_RETRY_TIMEOUT_MS);
    Serial.print(".");
    attempts++;
  }

  if (mqttClient.connected()) {
    Serial.println();
    Serial.println("MQTT connected");
    // Send all devices after connecting
    publishAllDevices();
  } else {
    Serial.println();
    Serial.println("MQTT connection failed");
  }
}

class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveManufacturerData()) {
      std::string mData = advertisedDevice.getManufacturerData();

      // Only save if ManufacturerData size is 9
      if (mData.size() == 9) {
        std::string mac = advertisedDevice.getAddress().toString();

        auto it = devices.find(mac);
        if (it == devices.end()) {
          // New device
          Serial.print("New device: ");
          Serial.print(mac.c_str());
          Serial.print(" Data: ");
          for (unsigned char c : mData) {
            Serial.printf("%02X ", (unsigned char)c);
          }
          Serial.println();
          devices[mac] = mData;
          publishDeviceData(mac, mData);
        } else if (it->second != mData) {
          // Data changed
          Serial.print("Data changed for device: ");
          Serial.print(mac.c_str());
          Serial.print(" Old: ");
          for (unsigned char c : it->second)
            Serial.printf("%02X ", (unsigned char)c);
          Serial.print(" New: ");
          for (unsigned char c : mData)
            Serial.printf("%02X ", (unsigned char)c);

          Serial.println();
          devices[mac] = mData;
          publishDeviceData(mac, mData);
        }
      }
    }
  }
};

void scanCompleteCB(BLEScanResults foundDevices) {
  pBLEScan->clearResults();
  scanning = false;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Yoswit BLE Scanner with WiFi & MQTT");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED OFF initially

  // Setup MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

  // Initialize BLE
  BLEDevice::init("Yoswit BLE Scanner");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // Active scan for more data

  Serial.println("Setup complete!");
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED)
    connectWiFi();

  // Maintain MQTT connection
  if (!mqttClient.connected()) {
    digitalWrite(LED_PIN, HIGH); // LED OFF when not connected
    connectMQTT();
  } else {
    digitalWrite(LED_PIN, LOW); // LED ON when connected
  }
  mqttClient.loop();

  // BLE scanning
  if (!scanning) {
    pBLEScan->start(SCAN_DURATION_SECONDS, scanCompleteCB, false);
    scanning = true;
  }
}