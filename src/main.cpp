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

// WiFi credentials
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// MQTT Broker settings
const char *MQTT_BROKER = "YOUR_MQTT_BROKER_IP";
const int MQTT_PORT = 1883;
const char *MQTT_CLIENT_ID = "YoswitBLEScanner";
const char *MQTT_TOPIC = "yoswit/ble/devices";

// Global variable to store ManufacturerData of size 9, keyed by MAC address
std::map<std::string, std::string> devices;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

BLEScan *pBLEScan;
bool scanning = false;

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_CONNECTION_ATTEMPTS) {
    delay(CONNECTION_RETRY_TIMEOUT_MS);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
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

void reconnectMQTT() {
  int attempts = 0;
  while (!mqttClient.connected() && attempts < MAX_CONNECTION_ATTEMPTS) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
      // Send all devices after connecting
      publishAllDevices();
      break;
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.print(" trying again in ");
      Serial.print(CONNECTION_RETRY_TIMEOUT_MS / 1000);
      Serial.println(" seconds");
      delay(CONNECTION_RETRY_TIMEOUT_MS);
      attempts++;
    }
  }
  if (!mqttClient.connected()) {
    Serial.println("MQTT connection failed after max retries!");
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
          for (unsigned char c : it->second) {
            Serial.printf("%02X ", (unsigned char)c);
          }
          Serial.print(" New: ");
          for (unsigned char c : mData) {
            Serial.printf("%02X ", (unsigned char)c);
          }
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

  // Connect to WiFi
  connectWiFi();

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
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    connectWiFi();
  }

  // Maintain MQTT connection
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // BLE scanning
  if (!scanning) {
    pBLEScan->start(SCAN_DURATION_SECONDS, scanCompleteCB, false);
    scanning = true;
  }
}