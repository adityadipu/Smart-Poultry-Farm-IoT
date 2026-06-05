# 📱 Android Application

The Smart Poultry Farm Android application provides real-time monitoring, automated control, analytics, and configuration management for the Smart Poultry Farm IoT system.

---

## Features

### 🔐 Secure Login

Users can securely access the poultry monitoring system through the login interface.

![Login Screen](app/screenshots/login.png)

---

### 📊 Live Dashboard

Monitor real-time sensor readings from the poultry farm, including:

* Temperature
* Humidity
* Air Quality
* Water Level
* Feed Level
* Light Level
* Battery Voltage
* System Current

![Dashboard](app/screenshots/dashboard.png)

---

### ⚙️ Actuator Controls

Manually control farm equipment or switch between automatic and manual operation modes.

Supported actuators:

* Feeder
* Fan
* Heater
* Lights
* Water Pump
* Ventilation
* Power Source Selection

![Controls](app/screenshots/controls.png)

---

### 📈 Analytics Dashboard

View historical trends and performance metrics collected from the poultry farm.

#### Analytics Overview

![Analytics Overview](app/screenshots/analytics_overview.png)

#### Analytics Charts

![Analytics Charts](app/screenshots/analytics_charts.png)

---

### 🔧 Configuration Settings

Configure environmental thresholds and automation parameters.

#### Temperature & Humidity Settings

![Configuration 1](app/screenshots/configure_thresholds_1.png)

#### Air Quality & Light Settings

![Configuration 2](app/screenshots/configure_thresholds_2.png)

#### Water Pump & Feeding Settings

![Configuration 3](app/screenshots/configure_thresholds_3.png)

---

### 🍽️ Feeding Schedule

Set automatic feeding schedules and feeding durations.

![Feeding Schedule](app/screenshots/feeding_schedule.png)

---

### 🔋 Power Management

Configure automatic switching between Solar and AC power sources based on battery voltage thresholds.

![Power Settings](app/screenshots/power_settings.png)

---

### 🖥️ Web Dashboard

A companion web dashboard is available for desktop monitoring and control.

![Web Dashboard](app/screenshots/web_dashboard.png)

---

## Technology Stack

* Flutter
* Dart
* MQTT Communication
* ESP32-S3 Integration
* Real-Time Sensor Monitoring
* Automated Actuator Control

---

## Application Functions

| Function         | Description                                |
| ---------------- | ------------------------------------------ |
| Dashboard        | Displays live sensor data                  |
| Controls         | Manage actuators manually or automatically |
| Analytics        | View historical sensor trends              |
| Configure        | Set thresholds and automation rules        |
| Feeding Schedule | Configure feeding times                    |
| Power Management | Solar/AC switching configuration           |

---

## Integration

The Android application communicates with the ESP32-S3 controller through MQTT to provide:

* Real-time monitoring
* Remote control
* Configuration management
* Historical analytics
* Automation parameter updates

---

## Screenshots Directory

```text
android-app/
└── app/
    └── screenshots/
        ├── login.png
        ├── dashboard.png
        ├── controls.png
        ├── analytics_overview.png
        ├── analytics_charts.png
        ├── configure_thresholds_1.png
        ├── configure_thresholds_2.png
        ├── configure_thresholds_3.png
        ├── feeding_schedule.png
        ├── power_settings.png
        └── web_dashboard.png
```
