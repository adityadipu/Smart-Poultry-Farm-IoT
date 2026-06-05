# Smart Poultry Farm IoT

An IoT-based Smart Poultry Farm Monitoring and Automation System built using ESP32-S3, MQTT communication, Android application, and intelligent power management.

---

## Project Overview

The Smart Poultry Farm IoT system provides real-time monitoring and automated control of poultry farm environmental conditions.

The system continuously monitors:

* Temperature
* Humidity
* Air Quality
* Water Level
* Feed Level
* Light Intensity
* Battery Voltage
* System Current

Based on configurable thresholds, the system automatically controls:

* Ventilation Fan
* Heater
* Water Pump
* Lighting System
* Automatic Feeder
* Solar / AC Power Switching

---

## Features

### Real-Time Monitoring

* Temperature Monitoring
* Humidity Monitoring
* Air Quality Monitoring
* Water Level Monitoring
* Feed Level Monitoring
* Light Level Monitoring
* Battery Voltage Monitoring
* System Current Monitoring

### Automated Control

* Fan Control
* Heater Control
* Water Pump Control
* Lighting Control
* Ventilation Control
* Automatic Feeding System

### Android Application

* Secure Login
* Live Dashboard
* Actuator Controls
* Historical Analytics
* Configuration Settings
* Feeding Schedule Management
* Power Management Settings

### Power Management

* Solar / AC Hybrid Operation
* Automatic Source Switching
* Battery Monitoring
* Voltage-Based Power Selection

### Communication

* MQTT Protocol
* Real-Time Data Synchronization
* Remote Monitoring and Control

---

# System Architecture

![System Architecture](docs/system_architecture.png)

---

# Firmware Design

### Main Firmware Flowchart

![Firmware Flowchart](docs/firmware_flowchart.png)

### App-Firmware Communication Flow

![App Firmware Flowchart](docs/App_firmware_flowchart.png)

### Power Management Flow

![Power Management Flowchart](docs/power_management_flowchart.png)

---

# Circuit Design

![Circuit Schematic](docs/circuit_schematic.png)

---

# Android Application

## Login Screen

![Login](android-app/app/screenshots/login.png)

## Dashboard

![Dashboard](android-app/app/screenshots/dashboard.png)

## Actuator Controls

![Controls](android-app/app/screenshots/controls.png)

## Analytics Overview

![Analytics](android-app/app/screenshots/analytics_overview.png)

## Analytics Charts

![Analytics Charts](android-app/app/screenshots/analytics_charts.png)

## Sensor Configuration

![Configuration](android-app/app/screenshots/configure_thresholds_1.png)

![Configuration](android-app/app/screenshots/configure_thresholds_2.png)

![Configuration](android-app/app/screenshots/configure_thresholds_3.png)

## Feeding Schedule

![Feeding Schedule](android-app/app/screenshots/feeding_schedule.png)

## Power Settings

![Power Settings](android-app/app/screenshots/power_settings.png)

## Web Dashboard

![Web Dashboard](android-app/app/screenshots/web_dashboard.png)

---

# Hardware

## Poultry Chamber Layout

![Poultry Chamber](hardware/sensors/poultry_chamber_layout.jpg)

## Water Level Sensor

![Water Sensor](hardware/watering/water_sensor.jpg)

## Feeder Mechanism

![Feeder](hardware/feeder/feeder_mechanism.jpg)

## Enclosure Exterior

![Exterior](hardware/enclosure/exterior.jpg)

## Enclosure Interior

![Interior](hardware/enclosure/interior.jpg)

---

# Technology Stack

## Hardware

* ESP32-S3
* Temperature & Humidity Sensors
* Air Quality Sensor
* Water Level Sensor
* Feed Level Sensor
* Light Sensor
* Relay Modules
* Solar Power System
* Battery Storage System

## Software

* Arduino IDE
* ESP32 Framework
* MQTT
* Android Application
* Web Dashboard

---

# Repository Structure

```text
Smart-Poultry-Farm-IoT
│
├── docs/
├── hardware/
├── firmware/
│   └── ESP32-S3/
│
└── android-app/
    └── app/
        └── screenshots/
```

---

# Research Documentation

Additional documentation and project resources are available in the `docs` directory.

* System Architecture
* Firmware Flowcharts
* Circuit Schematics
* Methodology Documentation

---

# Future Improvements

* Cloud Database Integration
* AI-Based Disease Detection
* Predictive Feed Management
* Weather Forecast Integration
* Multi-Farm Monitoring
* Mobile Push Notifications

---

# License

This project is licensed under the MIT License.

See the LICENSE file for details.
