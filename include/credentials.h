#ifndef CREDENTIALS_H
#define CREDENTIALS_H

// WiFi credentials
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// MQTT Broker settings
const char *MQTT_BROKER = "YOUR_MQTT_BROKER_IP";
const int MQTT_PORT = 1883;
const char *MQTT_CLIENT_ID = "YoswitBLEScanner";
const char *MQTT_TOPIC = "yoswit/ble/devices";

#endif // CREDENTIALS_H
