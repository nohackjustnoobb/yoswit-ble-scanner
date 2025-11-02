# Yoswit BLE Scanner

> **⚠️ Unofficial Implementation**: This is an unofficial, community-developed ESP32-based BLE scanner for Yoswit devices. It is not affiliated with, endorsed by, or officially supported by Yoswit.

An ESP32-based BLE scanner for [yoswit-homebridge-mqtt](https://github.com/nohackjustnoobb/yoswit-homebridge-mqtt.git) that keeps device status in sync when using physical buttons.

**Note:** Without this module, device status will not update in Homebridge if you use the physical button on the device.

## Prerequisites

- ESP32 development board
- PlatformIO IDE
- [yoswit-homebridge-mqtt](https://github.com/nohackjustnoobb/yoswit-homebridge-mqtt.git) running

## Installation

1. **Clone the repository**:

   ```bash
   git clone https://github.com/nohackjustnoobb/yoswit-ble-scanner.git
   cd yoswit-ble-scanner
   ```

2. **Configure credentials**:

   Edit `src/main.cpp`:

   ```cpp
   const char *WIFI_SSID = "YOUR_WIFI_SSID";
   const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
   const char *MQTT_BROKER = "YOUR_MQTT_BROKER_IP";  // IP of machine running yoswit-homebridge-mqtt
   ```

3. **Upload to ESP32**:

   ```bash
   platformio run --target upload
   ```

## Usage

The scanner will automatically:

- Connect to WiFi and MQTT broker
- Scan for Yoswit BLE devices
- Publish device status changes to topic `yoswit/ble/devices`
- The yoswit-homebridge-mqtt bridge will receive and process these updates

## License

MIT
