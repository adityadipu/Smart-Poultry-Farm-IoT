# ESP32-S3 Firmware

This directory contains the firmware for the Smart Poultry Farm IoT system.

The firmware is developed for the ESP32-S3 microcontroller and is responsible for sensor monitoring, actuator control, MQTT communication, and power management.

---

## Main File

```text
ESP32-S3/
└── platform.ino
```

---

## Features

### Sensor Monitoring

The firmware continuously reads data from:

* Temperature Sensor
* Humidity Sensor
* Air Quality Sensor
* Water Level Sensor
* Feed Level Sensor
* Light Sensor
* Battery Voltage Sensor
* Current Sensor

### Automated Control

Based on configurable thresholds, the firmware automatically controls:

* Ventilation Fan
* Heater
* Water Pump
* Lighting System
* Automatic Feeder
* Power Source Selection

### MQTT Communication

The ESP32-S3 communicates with the Android application and web dashboard using MQTT.

Functions include:

* Publishing sensor data
* Receiving control commands
* Updating configuration settings
* Synchronizing actuator status

### Power Management

The system supports hybrid power operation:

* Solar Power
* AC Power
* Battery Monitoring
* Automatic Source Switching

---

## Firmware Workflow

1. Initialize sensors and actuators.
2. Connect to Wi-Fi.
3. Connect to MQTT broker.
4. Read sensor data.
5. Publish sensor values.
6. Check threshold settings.
7. Execute automatic control actions.
8. Process incoming MQTT commands.
9. Update actuator states.
10. Repeat continuously.

---

## Communication Topics

### Published Topics

* Temperature
* Humidity
* Air Quality
* Water Level
* Feed Level
* Light Level
* Battery Voltage
* System Current

### Subscribed Topics

* Fan Control
* Heater Control
* Water Pump Control
* Light Control
* Ventilation Control
* Feeder Control
* Configuration Updates

---

## Hardware Platform

* ESP32-S3
* Relay Modules
* Environmental Sensors
* Solar Power System
* Battery Storage System

---

## Development Environment

* Arduino IDE
* ESP32 Board Package
* MQTT Client Library
* Wi-Fi Library

---

## Author

Smart Poultry Farm IoT Project

ESP32-S3 Firmware Implementation
