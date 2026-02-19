# Gesture Control System – ESP32 + Ultrasonic Sensors + MQTT

Real-time gesture detection system using two ultrasonic sensors connected to an ESP32, with live visualization in a web dashboard via MQTT (HiveMQ).

---

## Overview

This project detects hand gestures using two ultrasonic sensors connected to an ESP32.

The ESP32:
- Measures distance from both sensors
- Detects gestures based on movement patterns
- Publishes sensor data and gesture events via MQTT

The Web Dashboard:
- Subscribes to MQTT topics
- Displays live sensor distances
- Shows detected gestures
- Visualizes data in real-time

---

## Technologies Used

- ESP32
- Ultrasonic Sensors (HC-SR04)
- MQTT (HiveMQ Cloud)
- WebSockets
- HTML / CSS / JavaScript
- Real-time dashboard visualization

---

## ⚙️ Setup Instructions

### 1️⃣ ESP32 Configuration

Before uploading the firmware, you must provide your own credentials.

Open the ESP32 file and replace:

```cpp
const char* mqtt_server = "YOUR_MQTT_HOST";
const char* mqtt_username = "YOUR_USERNAME";
const char* mqtt_password = "YOUR_PASSWORD";
```

Also configure your Wi-Fi:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```
Then upload the code to the ESP32.

### 2️⃣ Frontend Configuration

In the web dashboard script, replace:

```cpp
const HOST = "YOUR_MQTT_HOST";
const USERNAME = "YOUR_USERNAME";
const PASSWORD = "YOUR_PASSWORD";
```
with your own HiveMQ credentials.



