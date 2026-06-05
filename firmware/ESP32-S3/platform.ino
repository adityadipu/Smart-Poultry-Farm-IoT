/*
 * Smart Poultry Farm - WIFI VERSION (ESP32-S3)
 * WIFI SSID: DNS
 * WIFI PASS: @LIONASHISH@
 * * Replaces SIM900A with ESP32 Native WiFi
 */

// --- Include Libraries ---
#include <WiFi.h>             // Replaces TinyGsmClient
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <ESP32Servo.h>
#include <math.h>
#include <Preferences.h> // For NVS
#include <Wire.h>        // For I2C
#include "RTClib.h"      // For DS3231 RTC

// --- WiFi Credentials ---
const char* ssid = "DNS";
const char* password = "@LIONASHISH@";

// --- MQTT Objects ---
WiFiClient espClient;         // Native WiFi Client
PubSubClient mqtt(espClient);

// --- NVS Object ---
Preferences preferences;

// --- RTC Object ---
RTC_DS3231 rtc;
TwoWire I2C_RTC = TwoWire(0); // I2C bus 0

// --- Time Variables ---
bool timeSynchronized = false;
char timeBuffer[30];

// --- MQTT Configuration ---
const char* mqtt_server = "broker.hivemq.com";
const int   mqtt_port   = 1883;
const char* mqtt_id     = "PoultryFarm_ESP32_S3_WIFI_001";
const char* topic_sensors = "poultry/farm001/sensors";
const char* topic_status  = "poultry/farm001/status";
const char* topic_control = "poultry/farm001/control";
const char* topic_config_set = "poultry/farm001/config/set";

// --- Calibration Range Constants ---
const int LDR_MIN_DARK = 10;
const int LDR_MAX_BRIGHT = 3000;
const int WATER_MIN_EMPTY = 17;
const int WATER_MAX_FULL = 700;

// --- Sensor/Actuator Pin Definitions ---
#define DHTPIN 4
#define DHTTYPE DHT22
#define LDR_PIN 6
#define WATER_LEVEL_PIN 7
#define MQ135_PIN 8
#define SERVO_PIN 14
#define FEED_TRIG_PIN 39
#define FEED_ECHO_PIN 40
#define FAN_RELAY_PIN 5
#define HEATER_RELAY_PIN 12
#define LIGHT_RELAY_PIN 13
#define PUMP_RELAY_PIN 15
#define VENT_FAN_RELAY_PIN 16
#define I2C_SDA_PIN 10
#define I2C_SCL_PIN 11

// --- Power System Pin Definitions ---
#define VOLTAGE_PIN 2    // Pin to read 12V battery level
#define CURRENT_PIN 3    // Pin to read current draw
#define RELAY_PIN 21     // Pin to control 12V transfer relay
#define PV_VOLTAGE_PIN 9 // Pin to read Solar Panel voltage

// --- Thresholds (Variables loaded from NVS) ---
float hysteresis = 2.0;
float tempCoolOn = 28.0;
float tempCoolOff = tempCoolOn - hysteresis;
float tempHeatOn = 20.0;
float tempHeatOff = tempHeatOn + hysteresis;
int lightThresholdOn = 1000;
int lightThresholdOff = 2000;
int waterLevelLowThreshold = 200;
int waterLevelHighThreshold = 600;
const float FEEDER_FULL = 4.0;
const float FEEDER_EMPTY = 10.0;
float airQualityThresholdOn  = 800.0;  
float airQualityThresholdOff = 600.0;  
float lowFeedThreshold = 10.0;

// --- Power System Calibration ---
const float VOLTAGE_DIVIDER_FACTOR_1 = 5.0;   
const float VOLTAGE_DIVIDER_FACTOR_2 = (10000.0 + 20000.0) / 20000.0; 
const float ADC_MAX = 4095.0; 
const float V_REF = 3.3;      
const float VOLTAGE_CALIBRATION_FACTOR = 0.80; 

// --- ACS712 Current Sensor Configuration ---
const float ACS712_SENSITIVITY = 0.100; 
const float ACS712_QUIESCENT_VOLTAGE = 2.483; 
const float CURRENT_CALIBRATION = 0.4228;     
const float CURRENT_DEAD_ZONE = 15.0;         

// --- Power Logic Thresholds ---
float voltageSwitchToSolar = 13.0; 
float voltageSwitchToAC = 12.0;    

// --- MQ135 Constants ---
#define RL_MQ135   1000.0
#define R0_MQ135   3718.65
#define MQ135_a    116.6020682
#define MQ135_b    -2.769034857
#define ADC_MAX_VAL 4095.0
#define V_IN        3.3

// --- Feeder Configuration ---
Servo feederServo;
#define FEED_CLOSED_ANGLE 45
#define FEED_OPEN_ANGLE 0

// --- Feeding Schedule Variables ---
int feedTimesCount = 2;
int feedDurationSeconds = 5;
int feedHours[4] = {8, 17, -1, -1};
int feedMinutes[4] = {0, 0, -1, -1};

// --- Task Timing ---
unsigned long lastSensorRead = 0;
const long SENSOR_READ_INTERVAL = 5000;
unsigned long lastMqttPublish = 0;
const long MQTT_PUBLISH_INTERVAL = 30000UL;
unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastWifiCheck = 0;           // Renamed from GSM
const long WIFI_CHECK_INTERVAL = 30000UL;  // Renamed from GSM
unsigned long lastScheduleCheck = 0;
const long SCHEDULE_CHECK_INTERVAL = 30000UL;

// --- Sensor Objects ---
DHT dht(DHTPIN, DHTTYPE);

// --- Global State Variables ---
float g_humidity = 0;
float g_tempC = 0;
int   g_ldrValue = 0;
int   g_waterValue = 0;
float g_airQualityPPM = 0;
float g_feedPercent = 0;
int g_signalQuality = 0; // Will be RSSI
int g_signalBars = 0;
String g_signalLevel = "none";
bool wifiConnected = false; // Renamed from gprsConnected
bool mqttConnected = false;
bool isFanOn = false;
bool isHeaterOn = false;
bool isLightOn = false;
bool isPumpOn = false;
bool isVentFanOn = false;
bool isFeeding = false;
unsigned long feedOpenStartTime = 0;
bool isFanAuto = true;
bool isHeaterAuto = true;
bool isLightAuto = true;
bool isPumpAuto = true;
bool isVentFanAuto = true;
bool isFeedTimerAuto = true;
bool configUpdatePending = false;
bool isScheduledFeed = false;

// --- Power System Global Variables ---
bool isUsingSolar = false;    
float g_batteryVoltage = 0.0; 
float g_current_mA = 0.0;     
int g_powerMode = 0;          
float g_pvVoltage = 0.0;  
bool g_isCharging = false;  

// --- FUNCTION PROTOTYPES ---
void startFeeding(bool isScheduled = false);
void stopFeeding();
void checkFeedingSchedule();
void checkFeedLevelAlert();
float getMq135Ppm();
float getUltrasonicDistance(int trigPin, int echoPin);
float readBatteryVoltage();
float readCurrent();
void controlPowerSystem();
void debugVoltageSensor();
void setup_wifi();          // New function
void calculateSignalStrength(); // Modified for WiFi

// ==================== NVS FUNCTIONS ====================
void loadSettingsFromNVS() {
    Serial.println("⚙️ Loading settings from NVS...");
    if (!preferences.begin("poultry-cfg", false)) {
        Serial.println("❌ Failed to initialize NVS! Using defaults.");
        return;
    }
    tempCoolOn = preferences.getFloat("high_temp", tempCoolOn);
    tempHeatOn = preferences.getFloat("low_temp", tempHeatOn);
    tempCoolOff = tempCoolOn - hysteresis;
    tempHeatOff = tempHeatOn + hysteresis;
    airQualityThresholdOn = preferences.getFloat("high_air", airQualityThresholdOn);
    airQualityThresholdOff = airQualityThresholdOn - 200.0;  
    waterLevelLowThreshold = preferences.getInt("water_low", waterLevelLowThreshold);
    waterLevelHighThreshold = preferences.getInt("water_high", waterLevelHighThreshold);
    lightThresholdOn = preferences.getInt("light_on", lightThresholdOn);
    lightThresholdOff = preferences.getInt("light_off", lightThresholdOff);
    lowFeedThreshold = preferences.getFloat("feed_low", lowFeedThreshold);

    // --- NEW: Load Power Settings ---
    voltageSwitchToSolar = preferences.getFloat("power_solar_v", voltageSwitchToSolar);
    voltageSwitchToAC = preferences.getFloat("power_ac_v", voltageSwitchToAC);
    g_powerMode = preferences.getInt("power_mode", 0); 
    
    feedTimesCount = preferences.getInt("feed_count", feedTimesCount);
    feedDurationSeconds = preferences.getInt("feed_dur", feedDurationSeconds);

    for (int i = 0; i < 4; i++) {
         feedHours[i] = preferences.getInt(("feed_h" + String(i)).c_str(), feedHours[i]);
         feedMinutes[i] = preferences.getInt(("feed_m" + String(i)).c_str(), feedMinutes[i]);
    }
    preferences.end();
    Serial.println("✅ NVS settings loaded.");
}

void saveSettingsToNVS() {
    Serial.println("💾 Saving configuration changes to NVS...");
    if (!preferences.begin("poultry-cfg", false)) {
        Serial.println("❌ Failed to open NVS for saving!");
        configUpdatePending = false;
        return;
    }
    preferences.putFloat("high_temp", tempCoolOn);
    preferences.putFloat("low_temp", tempHeatOn);
    preferences.putFloat("high_air", airQualityThresholdOn);
    preferences.putInt("water_low", waterLevelLowThreshold);
    preferences.putInt("water_high", waterLevelHighThreshold);
    preferences.putInt("light_on", lightThresholdOn);
    preferences.putInt("light_off", lightThresholdOff);
    preferences.putFloat("feed_low", lowFeedThreshold);
    
    // --- NEW: Save Power Settings ---
    preferences.putFloat("power_solar_v", voltageSwitchToSolar);
    preferences.putFloat("power_ac_v", voltageSwitchToAC);
    preferences.putInt("power_mode", g_powerMode);

    preferences.putInt("feed_count", feedTimesCount);
    preferences.putInt("feed_dur", feedDurationSeconds);
    for (int i = 0; i < 4; i++) {
        preferences.putInt(("feed_h" + String(i)).c_str(), feedHours[i]);
        preferences.putInt(("feed_m" + String(i)).c_str(), feedMinutes[i]);
    }
    preferences.end();
    Serial.println("✅ Settings saved to NVS.");
    configUpdatePending = false;
}

// ==================== TIME FUNCTIONS ====================
bool parseTimeString(const char* timeStr, int &hour, int &minute) {
    if (strlen(timeStr) < 4) return false;
    int h = -1, m = -1;
    char ampm[3] = "";
    int parsedItems = sscanf(timeStr, "%d:%d %2s", &h, &m, ampm);
    if (parsedItems == 3) {
        if (h < 1 || h > 12 || m < 0 || m > 59) return false;
        if ((strcmp(ampm, "PM") == 0 || strcmp(ampm, "pm") == 0) && h != 12) h += 12;
        if ((strcmp(ampm, "AM") == 0 || strcmp(ampm, "am") == 0) && h == 12) h = 0;
        hour = h; minute = m;
        return true;
    } else {
        parsedItems = sscanf(timeStr, "%d:%d", &h, &m);
        if (parsedItems == 2) {
            if (h < 0 || h > 23 || m < 0 || m > 59) return false;
            hour = h; minute = m;
            return true;
        }
    }
    return false;
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    Serial.println("==========================================");
    Serial.println("🐔 SMART POULTRY FARM - WIFI EDITION");
    Serial.println("Broker: broker.hivemq.com");
    Serial.println("==========================================");
    delay(1000);
    loadSettingsFromNVS();

    // --- Set up ADC resolution ---
    analogReadResolution(12); // Set ADC to 12-bit (0-4095)

    Serial.println("Initializing RTC module...");
    if (!I2C_RTC.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
        Serial.println("❌ Failed to initialize I2C bus for RTC!");
    }
    if (!rtc.begin(&I2C_RTC)) {
        Serial.println("❌ Couldn't find RTC module!");
        timeSynchronized = false;
    } else {
        if (rtc.lostPower() || rtc.now().year() < 2023) {
            Serial.println("RTC lost power or time is invalid!");
            timeSynchronized = false;
        } else {
            Serial.println("✅ RTC is running.");
            timeSynchronized = true;
        }
    }

    delay(1000);
    initializeHardware(); // Initializes all farm pins
    
    // --- Initialize Power System Hardware ---
    Serial.println("Initializing power system...");
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Start in fail-safe mode (AC Adapter)
    isUsingSolar = false;
    
    // Debug voltage sensor calibration
    debugVoltageSensor();
    
    // --- Initialize WiFi ---
    setup_wifi();
    
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setCallback(mqttCallback);
    mqtt.setKeepAlive(90);
    mqtt.setSocketTimeout(60);
    mqtt.setBufferSize(1024);
    Serial.println("✅ SYSTEM INITIALIZATION COMPLETE");
    Serial.println("==========================================");
}

// ==================== MAIN LOOP ====================
void loop() {
    unsigned long currentMillis = millis();
    
    // --- Check WiFi Connection ---
    if (WiFi.status() != WL_CONNECTED) {
        wifiConnected = false;
        setup_wifi(); 
    } else {
        wifiConnected = true;
    }

    if (wifiConnected) {
        if (!mqtt.connected()) {
            mqttConnected = false;
            connectMQTT();
        } else {
            mqtt.loop();
            mqttConnected = true;
        }
    }

    if (configUpdatePending) {
        saveSettingsToNVS();
    }
    if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = currentMillis;
        runEnvironmentControls(); // This now runs sensor reads AND power logic
    }
    if (timeSynchronized && isFeedTimerAuto && (currentMillis - lastScheduleCheck >= SCHEDULE_CHECK_INTERVAL)) {
         lastScheduleCheck = currentMillis;
         checkFeedingSchedule();
    }
    if (isFeeding && isScheduledFeed && (currentMillis - feedOpenStartTime >= (unsigned long)feedDurationSeconds * 1000UL)) {
        stopFeeding();
    }
    if (wifiConnected && mqttConnected &&
        currentMillis - lastMqttPublish >= MQTT_PUBLISH_INTERVAL) {
        lastMqttPublish = currentMillis;
        publishSensorData();
    }
    delay(10);
}

// ==================== HARDWARE INITIALIZATION ====================
void initializeHardware() {
    Serial.println("Initializing hardware...");
    dht.begin();
    pinMode(FEED_TRIG_PIN, OUTPUT);
    pinMode(FEED_ECHO_PIN, INPUT);
    pinMode(FAN_RELAY_PIN, OUTPUT);
    pinMode(HEATER_RELAY_PIN, OUTPUT);
    pinMode(LIGHT_RELAY_PIN, OUTPUT);
    pinMode(PUMP_RELAY_PIN, OUTPUT);
    pinMode(VENT_FAN_RELAY_PIN, OUTPUT);
    digitalWrite(FAN_RELAY_PIN, HIGH);
    digitalWrite(HEATER_RELAY_PIN, HIGH);
    digitalWrite(LIGHT_RELAY_PIN, HIGH);
    digitalWrite(PUMP_RELAY_PIN, LOW);
    digitalWrite(VENT_FAN_RELAY_PIN, HIGH);
    feederServo.setPeriodHertz(50);
    feederServo.attach(SERVO_PIN);
    feederServo.write(FEED_CLOSED_ANGLE);
    Serial.println("✅ Hardware initialized");
}

// ==================== VOLTAGE READING FUNCTION ====================
float readBatteryVoltage() {
    static float voltageReadings[5] = {0};
    static int voltageIndex = 0;
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(VOLTAGE_PIN);
        delay(2);
    }
    float avgADC = (float)sum / 10.0;
    float voltageAtADC = (avgADC * V_REF) / ADC_MAX;
    float measuredVoltage = voltageAtADC * VOLTAGE_DIVIDER_FACTOR_1 * VOLTAGE_DIVIDER_FACTOR_2;
    float calibratedVoltage = measuredVoltage * VOLTAGE_CALIBRATION_FACTOR;
    voltageReadings[voltageIndex] = calibratedVoltage;
    voltageIndex = (voltageIndex + 1) % 5;
    float sumFiltered = 0;
    for (int i = 0; i < 5; i++) {
        sumFiltered += voltageReadings[i];
    }
    return sumFiltered / 5.0;
}

// ==================== DEBUG VOLTAGE SENSOR ====================
void debugVoltageSensor() {
    Serial.println("🔧 Debugging Voltage Sensor:");
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(VOLTAGE_PIN);
        delay(10);
    }
    int rawADC = sum / 10;
    float voltageAtADC = (rawADC * V_REF) / ADC_MAX;
    float calculatedVoltage = voltageAtADC * VOLTAGE_DIVIDER_FACTOR_1 * VOLTAGE_DIVIDER_FACTOR_2;
    float calibratedVoltage = calculatedVoltage * VOLTAGE_CALIBRATION_FACTOR;
    Serial.printf("  Raw ADC: %d\n", rawADC);
    Serial.printf("  Calibrated Voltage: %.3fV\n", calibratedVoltage);
}

// ==================== IMPROVED CURRENT READING ====================
float readCurrent() {
    static float currentReadings[10] = {0};
    static int currentIndex = 0;
    static unsigned long lastReading = 0;
    
    if (millis() - lastReading < 100) {
        return currentReadings[currentIndex]; 
    }
    lastReading = millis();
    
    long sum = 0;
    for (int i = 0; i < 50; i++) {
        sum += analogRead(CURRENT_PIN);
        delay(1);
    }
    float avgADC = (float)sum / 50.0;
    float voltage = (avgADC * V_REF) / ADC_MAX;
    float voltage_5V = voltage * VOLTAGE_DIVIDER_FACTOR_2;
    float rawCurrent = (voltage_5V - ACS712_QUIESCENT_VOLTAGE) / ACS712_SENSITIVITY;
    float current_mA = rawCurrent * 1000.0 * CURRENT_CALIBRATION;
    if (fabs(current_mA) < CURRENT_DEAD_ZONE) {
        current_mA = 0.0;
    }
    currentReadings[currentIndex] = current_mA;
    currentIndex = (currentIndex + 1) % 10;
    float filteredSum = 0;
    for (int i = 0; i < 10; i++) {
        filteredSum += currentReadings[i];
    }
    return filteredSum / 10.0;
}

// ==================== PV VOLTAGE READING ====================
float readPvVoltage() {
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(PV_VOLTAGE_PIN);
        delay(2);
    }
    float avgADC = (float)sum / 10.0;
    float voltageAtADC = (avgADC * V_REF) / ADC_MAX;
    float pvVoltage = voltageAtADC * ( (100000.0 + 10000.0) / 10000.0 );
    if (pvVoltage < 0.5) {
      return 0.0;
    }
    return pvVoltage;
}

// ==================== POWER SYSTEM CONTROL ====================
void controlPowerSystem() {
    switch (g_powerMode) {
        case 1: // Force Solar
            if (!isUsingSolar) {
                Serial.println("🌞 POWER: Manually FORCING SOLAR power");
                digitalWrite(RELAY_PIN, HIGH); // Switch to NO (Solar)
                isUsingSolar = true;
            }
            break;
        case 2: // Force AC
            if (isUsingSolar) {
                Serial.println("🔌 POWER: Manually FORCING AC ADAPTER power");
                digitalWrite(RELAY_PIN, LOW); // Switch to NC (AC)
                isUsingSolar = false;
            }
            break;
        case 0: // Auto mode 
        default:
            if (g_batteryVoltage > voltageSwitchToSolar && !isUsingSolar && g_batteryVoltage > 5.0) {
                Serial.println("🔋 POWER (Auto): Battery FULL! Switching to SOLAR power");
                digitalWrite(RELAY_PIN, HIGH); 
                isUsingSolar = true;
            } else if (g_batteryVoltage < voltageSwitchToAC && isUsingSolar && g_batteryVoltage > 5.0) {
                Serial.println("🔌 POWER (Auto): Battery LOW! Switching to AC ADAPTER power");
                digitalWrite(RELAY_PIN, LOW); 
                isUsingSolar = false;
            }
            break;
    }
}

// ==================== WIFI MANAGEMENT (NEW) ====================
void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if(WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("");
        Serial.println("✅ WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        calculateSignalStrength();
    } else {
        Serial.println("");
        Serial.println("❌ WiFi connection failed.");
        wifiConnected = false;
    }
}

// ==================== SIGNAL STRENGTH CALCULATION (WIFI) ====================
void calculateSignalStrength() {
    long rssi = WiFi.RSSI();
    g_signalQuality = rssi;
    
    // Map RSSI to Bars
    if (rssi > -55) { 
        g_signalBars = 5; 
        g_signalLevel = "excellent"; 
    } else if (rssi > -65) { 
        g_signalBars = 4; 
        g_signalLevel = "good"; 
    } else if (rssi > -75) { 
        g_signalBars = 3; 
        g_signalLevel = "fair"; 
    } else if (rssi > -85) { 
        g_signalBars = 2; 
        g_signalLevel = "poor"; 
    } else if (rssi > -96) { 
        g_signalBars = 1; 
        g_signalLevel = "very_poor"; 
    } else { 
        g_signalBars = 0; 
        g_signalLevel = "no_signal"; 
    }
}

// ==================== MQTT MANAGEMENT ====================
void connectMQTT() {
  if (millis() - lastMqttReconnectAttempt < 5000) { return; }
  lastMqttReconnectAttempt = millis();
  Serial.print("Connecting to MQTT (broker.hivemq.com)...");
  
  if (mqtt.connect(mqtt_id, NULL, NULL, topic_status, 1, true, "{\"status\":\"offline\"}")) {
    Serial.println("✅ Connected to HiveMQ");
    delay(100);
    mqtt.subscribe(topic_control);
    Serial.println("✅ Subscribed to control topic");
    mqtt.subscribe(topic_config_set);
    Serial.println("✅ Subscribed to config topic");
    publishStatus();
  } else {
    Serial.print("❌ HiveMQ connection failed, rc=");
    Serial.println(mqtt.state());
    Serial.println("Will retry in 5 seconds...");
  }
}

void publishStatus() {
  if (wifiConnected) { calculateSignalStrength(); }
  char statusMsg[150];
  if (wifiConnected) {
    snprintf(statusMsg, sizeof(statusMsg), "{\"status\":\"online\",\"signal\":%d,\"signal_bars\":%d,\"signal_level\":\"%s\",\"ip\":\"%s\"}", g_signalQuality, g_signalBars, g_signalLevel.c_str(), WiFi.localIP().toString().c_str());
  } else {
    snprintf(statusMsg, sizeof(statusMsg), "{\"status\":\"offline\",\"signal\":0,\"signal_bars\":0,\"signal_level\":\"none\",\"ip\":\"none\"}");
  }
  for (int i = 0; i < 3; i++) {
    if (mqtt.publish(topic_status, statusMsg)) {
      Serial.println("✅ Status published");
      break;
    } else {
      Serial.println("❌ Status publish failed, retrying...");
      delay(1000);
    }
  }
}

// --- mqttCallback (Standard) ---
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("📨 MQTT Message: ");
    Serial.print(topic);
    Serial.print(" - ");
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    Serial.println(message);

    if (strcmp(topic, topic_config_set) == 0) {
        Serial.println("⚙️ Received configuration update. Parsing...");
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, message);
        if (error) {
            Serial.print("❌ deserializeJson() failed: ");
            Serial.println(error.c_str());
            return;
        }
        
        // --- Power Mode Parsing ---
        if (doc.containsKey("power_mode")) {
            int newMode = doc["power_mode"].as<int>();
            if (newMode >= 0 && newMode <= 2 && newMode != g_powerMode) {
                g_powerMode = newMode;
                configUpdatePending = true;
            }
        }
        if (doc.containsKey("power_ac_v")) {
            float newAcV = doc["power_ac_v"].as<float>();
            if (abs(newAcV - voltageSwitchToAC) > 0.01) {
                voltageSwitchToAC = newAcV;
                configUpdatePending = true;
            }
        }
        if (doc.containsKey("power_solar_v")) {
            float newSolarV = doc["power_solar_v"].as<float>();
            if (abs(newSolarV - voltageSwitchToSolar) > 0.01) {
                voltageSwitchToSolar = newSolarV;
                configUpdatePending = true;
            }
        }
        
        if (doc.containsKey("high_temp")) {
            float newHighTemp = doc["high_temp"].as<float>();
            if (abs(newHighTemp - tempCoolOn) > 0.01) {
                tempCoolOn = newHighTemp;
                tempCoolOff = tempCoolOn - hysteresis;
                configUpdatePending = true;
            }
        }
        if (doc.containsKey("low_temp")) {
            float newLowTemp = doc["low_temp"].as<float>();
            if (abs(newLowTemp - tempHeatOn) > 0.01) {
                tempHeatOn = newLowTemp;
                tempHeatOff = tempHeatOn + hysteresis;
                configUpdatePending = true;
            }
        }
        if (doc.containsKey("high_air")) {
            float newHighAir = doc["high_air"].as<float>();
            if (abs(newHighAir - airQualityThresholdOn) > 0.01) {
                airQualityThresholdOn = newHighAir;
                airQualityThresholdOff = airQualityThresholdOn - 200.0;   
                configUpdatePending = true;
            }
        }
         if (doc.containsKey("water_low")) {
            int newLowWater = doc["water_low"].as<int>(); 
            if (newLowWater != waterLevelLowThreshold) {
                waterLevelLowThreshold = newLowWater;
                configUpdatePending = true;
            }
         }
         if (doc.containsKey("water_high")) {
             int newHighWater = doc["water_high"].as<int>();
             if (newHighWater != waterLevelHighThreshold) {
                waterLevelHighThreshold = newHighWater;
                configUpdatePending = true;
             }
         }
         if (doc.containsKey("light_on")) {
             int newLightOn = doc["light_on"].as<int>();
             if (newLightOn != lightThresholdOn) {
                lightThresholdOn = newLightOn;
                configUpdatePending = true;
             }
         }
         if (doc.containsKey("light_off")) {
             int newLightOff = doc["light_off"].as<int>();
             if (newLightOff != lightThresholdOff) {
                lightThresholdOff = newLightOff;
                configUpdatePending = true;
             }
         }
         if (doc.containsKey("feed_low")) {
             float newFeedLow = doc["feed_low"].as<float>();
             if (abs(newFeedLow - lowFeedThreshold) > 0.01) {
                lowFeedThreshold = newFeedLow;
                configUpdatePending = true;
             }
         }
        if (doc.containsKey("feed_count")) {
            int newCount = doc["feed_count"].as<int>();
            newCount = constrain(newCount, 0, 4);
            if (newCount != feedTimesCount) {
                feedTimesCount = newCount;
                configUpdatePending = true;
            }
        }
        if (doc.containsKey("feed_duration")) {
            int newDuration = doc["feed_duration"].as<int>();
            if (newDuration > 0 && newDuration < 600 && newDuration != feedDurationSeconds) {
                feedDurationSeconds = newDuration;
                configUpdatePending = true;
            }
        }
        for (int i = 0; i < 4; i++) {
            String key = "feed_time_" + String(i + 1);
            bool timeSlotChanged = false;
            if (i < feedTimesCount && doc.containsKey(key.c_str())) {
                 const char* timeStr = doc[key.c_str()].as<const char*>();
                 int parsedHour = -1, parsedMin = -1;
                 if (parseTimeString(timeStr, parsedHour, parsedMin)) {
                      if (parsedHour != feedHours[i] || parsedMin != feedMinutes[i]) {
                          feedHours[i] = parsedHour; feedMinutes[i] = parsedMin;
                          timeSlotChanged = true;
                      }
                 } else {
                       if(feedHours[i] != -1 || feedMinutes[i] != -1) {
                          feedHours[i] = -1; feedMinutes[i] = -1;
                          timeSlotChanged = true;
                       }
                 }
            } else {
                 if (feedHours[i] != -1 || feedMinutes[i] != -1) {
                     feedHours[i] = -1; feedMinutes[i] = -1;
                     timeSlotChanged = true;
                 }
            }
            if (timeSlotChanged) {
               configUpdatePending = true;
            }
        }
        if (configUpdatePending) { Serial.println("  Configuration pending save."); } 
        else { Serial.println("  No settings changed, values are already current."); }
        return;
    } 
    if (strcmp(topic, topic_control) == 0) {
        processControlCommand(message);
    }
}

void processControlCommand(const char* command) {
    if (strcmp(command, "FEED_NOW") == 0) {
        Serial.println("🎯 REMOTE: Manual feed ON!");
        isFeedTimerAuto = false;
        startFeeding(false);
    }
    else if (strcmp(command, "FEED_STOP") == 0) {
        Serial.println("🎯 REMOTE: Manual feed STOP!");
        isFeedTimerAuto = false;
        stopFeeding();
    }
    else if (strcmp(command, "LIGHTS_ON") == 0) {
        Serial.println("💡 REMOTE: Turning lights ON (Manual)");
        isLightAuto = false;
        setLight(true);
    }
    else if (strcmp(command, "LIGHTS_OFF") == 0) {
        Serial.println("💡 REMOTE: Turning lights OFF (Manual)");
        isLightAuto = false;
        setLight(false);
    }
    else if (strcmp(command, "FAN_ON") == 0) {
        Serial.println("🌀 REMOTE: Turning fan ON (Manual)");
        isFanAuto = false;
        setFan(true);
    }
    else if (strcmp(command, "FAN_OFF") == 0) {
        Serial.println("🌀 REMOTE: Turning fan OFF (Manual)");
        isFanAuto = false;
        setFan(false);
    }
    else if (strcmp(command, "HEATER_ON") == 0) {
        Serial.println("🔥 REMOTE: Turning heater ON (Manual)");
        isHeaterAuto = false;
        setHeater(true);
    }
    else if (strcmp(command, "HEATER_OFF") == 0) {
        Serial.println("🔥 REMOTE: Turning heater OFF (Manual)");
        isHeaterAuto = false;
        setHeater(false);
    }
    else if (strcmp(command, "VENT_ON") == 0) {
        Serial.println("💨 REMOTE: Turning ventilation ON (Manual)");
        isVentFanAuto = false;
        setVentFan(true);
    }
    else if (strcmp(command, "VENT_OFF") == 0) {
        Serial.println("💨 REMOTE: Turning ventilation OFF (Manual)");
        isVentFanAuto = false;
        setVentFan(false);
    }
    else if (strcmp(command, "PUMP_ON") == 0) {
        Serial.println("💧 REMOTE: Turning pump ON (Manual)");
        isPumpAuto = false;
        setPump(true);
    }
    else if (strcmp(command, "PUMP_OFF") == 0) {
        Serial.println("💧 REMOTE: Turning pump OFF (Manual)");
        isPumpAuto = false;
        setPump(false);
    }
    else if (strcmp(command, "ALL_AUTO") == 0) {
        Serial.println("🤖 REMOTE: Setting ALL actuators to AUTO");
        isFanAuto = true;
        isHeaterAuto = true;
        isLightAuto = true;
        isPumpAuto = true;
        isVentFanAuto = true;
        isFeedTimerAuto = true;
        g_powerMode = 0; 
        configUpdatePending = true;
    }
    else if (strcmp(command, "STATUS") == 0) {
        Serial.println("📊 REMOTE: Status request");
        publishSensorData();
    }
    else if (strcmp(command, "POWER_AUTO") == 0) {
        Serial.println("🤖 POWER: Switching to AUTO mode.");
        g_powerMode = 0;
        configUpdatePending = true;
    }
    else if (strcmp(command, "POWER_SOLAR") == 0) {
        Serial.println("🌞 POWER: Forcing SOLAR mode.");
        g_powerMode = 1;
        configUpdatePending = true;
    }
    else if (strcmp(command, "POWER_AC") == 0) {
        Serial.println("🔌 POWER: Forcing AC ADAPTER mode.");
        g_powerMode = 2;
        configUpdatePending = true;
    }
}

// ==================== SENSOR DATA PUBLISHING ====================
void publishSensorData() {
    if (!mqttConnected || !mqtt.connected()) { return; }
    StaticJsonDocument<1024> doc; 
    doc["temp"] = g_tempC;
    doc["humid"] = g_humidity;
    doc["air_ppm"] = g_airQualityPPM;
    doc["light"] = g_ldrValue;
    doc["water"] = g_waterValue;
    doc["feed_level"] = g_feedPercent;
    doc["light_min"] = LDR_MIN_DARK;
    doc["light_max"] = LDR_MAX_BRIGHT;
    doc["water_min"] = WATER_MIN_EMPTY;
    doc["water_max"] = WATER_MAX_FULL;
    doc["temp_high_threshold"] = tempCoolOn;
    doc["temp_low_threshold"] = tempHeatOn;
    doc["air_high_threshold"] = airQualityThresholdOn;
    doc["light_on_threshold"] = lightThresholdOn;
    doc["light_off_threshold"] = lightThresholdOff;
    doc["water_low_threshold"] = waterLevelLowThreshold;
    doc["water_high_threshold"] = waterLevelHighThreshold;
    doc["feed_low_threshold"] = lowFeedThreshold;
    
    // Power Config
    doc["power_switch_to_ac"] = voltageSwitchToAC;
    doc["power_switch_to_solar"] = voltageSwitchToSolar;
    doc["power_mode"] = g_powerMode;
    doc["power_source"] = isUsingSolar ? "Solar" : "AC";
    doc["is_charging"] = g_isCharging;
    doc["battery_voltage"] = g_batteryVoltage;
    doc["current_ma"] = g_current_mA;
    doc["pv_voltage"] = g_pvVoltage;

    doc["fan"] = isFanOn;
    doc["heater"] = isHeaterOn;
    doc["light_on"] = isLightOn;
    doc["pump"] = isPumpOn;
    doc["vent_fan"] = isVentFanOn;
    doc["feeding"] = isFeeding;
    doc["fan_auto"] = isFanAuto;
    doc["heater_auto"] = isHeaterAuto;
    doc["light_auto"] = isLightAuto;
    doc["pump_auto"] = isPumpAuto;
    doc["vent_auto"] = isVentFanAuto;
    doc["feed_auto"] = isFeedTimerAuto;
    
    doc["wifi_connected"] = wifiConnected; // Renamed from gprs
    doc["mqtt_connected"] = mqttConnected;
    doc["signal"] = g_signalQuality;       // RSSI
    doc["signal_bars"] = g_signalBars;
    doc["signal_level"] = g_signalLevel;
    doc["ip"] = wifiConnected ? WiFi.localIP().toString() : "none";
    doc["time_synced"] = timeSynchronized;
    
    if (timeSynchronized) {
        DateTime now = rtc.now();
        sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        doc["current_time"] = timeBuffer;
    } else {
        doc["current_time"] = "unknown";
    }
    doc["feed_times_count"] = feedTimesCount;
    doc["feed_duration"] = feedDurationSeconds;
    JsonArray feedTimes = doc.createNestedArray("feed_schedule");
    for (int i = 0; i < feedTimesCount; i++) {
        if (feedHours[i] != -1 && feedMinutes[i] != -1) {
            char timeSlot[10];
            snprintf(timeSlot, sizeof(timeSlot), "%02d:%02d", feedHours[i], feedMinutes[i]);
            feedTimes.add(timeSlot);
        }
    }
    doc["timestamp"] = millis();
    char jsonBuffer[1024];
    size_t n = serializeJson(doc, jsonBuffer);
    Serial.printf("📊 JSON size: %d bytes\n", n);
    if (n >= sizeof(jsonBuffer)) {
        Serial.println("⚠️ WARNING: JSON too large for buffer!");
        return;
    }
    bool success = false;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (mqtt.publish(topic_sensors, jsonBuffer)) {
            success = true;
            Serial.println("📤 Data published to HiveMQ");
            break;
        } else {
            Serial.print("❌ Publish failed, attempt "); Serial.println(attempt + 1);
            delay(1000);
            if (!mqtt.connected()) {
                Serial.println("MQTT connection lost during publish");
                mqttConnected = false;
                break;
            }
        }
    }
    if (!success) {
        Serial.println("❌ All publish attempts failed");
        mqttConnected = false;
    }
}

// ==================== ENVIRONMENT CONTROL LOGIC ====================
void runEnvironmentControls() {
    readAllSensors();
    if (wifiConnected) { calculateSignalStrength(); }
    
    controlPowerSystem();
    const float CHARGING_VOLTAGE_THRESHOLD = 13.1; 
    if (g_batteryVoltage > CHARGING_VOLTAGE_THRESHOLD) {
      g_isCharging = true;
    } else {
      g_isCharging = false;
    }

    Serial.print("🌡️ Temp: "); Serial.print(g_tempC);
    Serial.print("°C | 💧 Humid: "); Serial.print(g_humidity);
    Serial.print("% | 💡 Light: "); Serial.print(g_ldrValue);
    Serial.print(" (Auto: "); Serial.print(isLightAuto ? "Y" : "N"); Serial.print(")");
    Serial.print(" | 💦 Water: "); Serial.print(g_waterValue);
    Serial.print(" (Auto: "); Serial.print(isPumpAuto ? "Y" : "N"); Serial.print(")");
    Serial.print(" | 💨 Air: "); Serial.print(g_airQualityPPM, 1);
    Serial.print(" ppm (Auto: "); Serial.print(isVentFanAuto ? "Y" : "N"); Serial.print(")");
    Serial.print(" | 🍗 Feed: "); Serial.print(g_feedPercent, 0);
    Serial.print("% (Auto: "); Serial.print(isFeedTimerAuto ? "Y" : "N"); Serial.print(")");
    
    Serial.print(" | 🔋 Battery: "); Serial.print(g_batteryVoltage, 2); Serial.print("V");
    Serial.print(" | ⚡ Current: "); Serial.print(g_current_mA, 0); Serial.print("mA");
    Serial.print(" | 🔌 Source: "); 
    if(g_powerMode == 1) Serial.print("Solar (Forced)");
    else if(g_powerMode == 2) Serial.print("AC (Forced)");
    else Serial.print(isUsingSolar ? "Solar (Auto)" : "AC (Auto)");
    
    if (g_feedPercent <= lowFeedThreshold) {
        Serial.print(" | ⚠️ LOW FEED ALERT!");
    }
    if (timeSynchronized) {
        DateTime now = rtc.now();
        sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        Serial.print(" | 🕒 Time: ");
        Serial.print(timeBuffer);
    }
    if (wifiConnected) {
        Serial.print(" | 📶 Signal: "); Serial.print(g_signalQuality);
        Serial.print("dBm | Bars: "); Serial.print(g_signalBars);
    } else {
        Serial.print(" | 📶 Signal: No WiFi");
    }
    Serial.println();
    
    controlTemperature();
    controlLight();
    controlWaterPump();
    controlAirQuality();
    checkFeedLevelAlert();
}

void readAllSensors() {
    g_humidity = dht.readHumidity();
    g_tempC = dht.readTemperature();
    g_ldrValue = analogRead(LDR_PIN);
    g_waterValue = analogRead(WATER_LEVEL_PIN);
    g_airQualityPPM = getMq135Ppm();
    float feedDistance = getUltrasonicDistance(FEED_TRIG_PIN, FEED_ECHO_PIN);
    g_feedPercent = 100.0 * (FEEDER_EMPTY - feedDistance) / (FEEDER_EMPTY - FEEDER_FULL);
    g_feedPercent = constrain(g_feedPercent, 0, 100);
    g_batteryVoltage = readBatteryVoltage();
    g_current_mA = readCurrent();
    g_pvVoltage = readPvVoltage();
}

void checkFeedLevelAlert() {
    if (g_feedPercent <= lowFeedThreshold) {
        static unsigned long lastAlertTime = 0;
        if (millis() - lastAlertTime >= 3600000) { 
            Serial.println("⚠️ ALERT: Feed level is LOW! Please refill.");
            lastAlertTime = millis();
            if (mqttConnected) {
                char alertMsg[100];
                snprintf(alertMsg, sizeof(alertMsg), 
                         "{\"alert\":\"low_feed\",\"level\":%.1f,\"threshold\":%.1f}",
                         g_feedPercent, lowFeedThreshold);
                mqtt.publish("poultry/farm001/alerts", alertMsg);
            }
        }
    }
}

void controlTemperature() {
  if (isnan(g_tempC)) return;
  if (isFanAuto) {
    if (g_tempC > tempCoolOn && !isFanOn) {
      Serial.println("🌡️ AUTO: Temperature HIGH! Turning FAN ON");
      setFan(true);
      if (isHeaterOn) { Serial.println("⚠️ SAFETY: Turning HEATER OFF"); setHeater(false); }
    }
    else if (isFanOn && g_tempC < tempCoolOff) {
      Serial.println("🌡️ AUTO: Temperature normal. Turning FAN OFF");
      setFan(false);
    }
  }
  if (isHeaterAuto) {
    if (g_tempC < tempHeatOn && !isHeaterOn) {
      Serial.println("🌡️ AUTO: Temperature LOW! Turning HEATER ON");
      setHeater(true);
      if (isFanOn) { Serial.println("⚠️ SAFETY: Turning FAN OFF"); setFan(false); }
    }
    else if (isHeaterOn && g_tempC > tempHeatOff) {
      Serial.println("🌡️ AUTO: Temperature normal. Turning HEATER OFF");
      setHeater(false);
    }
  }
}

void controlLight() {
  if (!isLightAuto) return;
  if (g_ldrValue < lightThresholdOn && !isLightOn) {
    Serial.println("🌙 AUTO: It's DARK. Turning lights ON");
    setLight(true);
  }
  else if (g_ldrValue > lightThresholdOff && isLightOn) {
    Serial.println("☀️ AUTO: It's LIGHT. Turning lights OFF");
    setLight(false);
  }
}

void controlWaterPump() {
  if (!isPumpAuto) return;
  if (g_waterValue < waterLevelLowThreshold && !isPumpOn) {
    Serial.println("💧 AUTO: Water level LOW! Turning PUMP ON");
    setPump(true);
  }
  else if (g_waterValue > waterLevelHighThreshold && isPumpOn) {
    Serial.println("💧 AUTO: Water level FULL. Turning PUMP OFF");
    setPump(false);
  }
}

void controlAirQuality() {
  if (!isVentFanAuto) return;
  if (g_airQualityPPM > airQualityThresholdOn && !isVentFanOn) {
    Serial.println("💨 AUTO: Air quality POOR! Turning VENTILATION ON");
    setVentFan(true);
  }
  else if (g_airQualityPPM < airQualityThresholdOff && isVentFanOn) {
    Serial.println("💨 AUTO: Air quality GOOD. Turning VENTILATION OFF");
    setVentFan(false);
  }
}

// ==================== ACTUATOR CONTROL FUNCTIONS ====================
void setFan(bool state) {
  digitalWrite(FAN_RELAY_PIN, state ? LOW : HIGH);
  isFanOn = state;
}

void setHeater(bool state) {
  digitalWrite(HEATER_RELAY_PIN, state ? LOW : HIGH);
  isHeaterOn = state;
}

void setLight(bool state) {
  digitalWrite(LIGHT_RELAY_PIN, state ? LOW : HIGH);
  isLightOn = state;
}

void setPump(bool state) {
  digitalWrite(PUMP_RELAY_PIN, state ? HIGH : LOW); // Inverted logic
  isPumpOn = state;
  Serial.print("💧 PUMP: "); Serial.print(state ? "ON" : "OFF");
  Serial.print(" | Relay Pin: "); Serial.println(state ? "HIGH" : "LOW");
}

void setVentFan(bool state) {
  digitalWrite(VENT_FAN_RELAY_PIN, state ? LOW : HIGH);
  isVentFanOn = state;
}

// ==================== FEEDER CONTROL ====================
void startFeeding(bool isScheduled) {
    if (isFeeding) {
        Serial.println("⚠️ FEEDING: Already feeding, ignoring new request");
        return;
    }
    Serial.printf("🔧 START FEEDING CALLED: scheduled=%d, time=%lu\n", isScheduled, millis());
    if (!feederServo.attached()) {
        Serial.println("🔄 Reattaching servo...");
        feederServo.attach(SERVO_PIN);
        delay(100);
    }
    isFeeding = true;
    isScheduledFeed = isScheduled;
    feedOpenStartTime = millis();
    Serial.printf("🎯 MOVING SERVO: PIN=%d, from %d° to %d°\n", 
                  SERVO_PIN, FEED_CLOSED_ANGLE, FEED_OPEN_ANGLE);
    feederServo.write(FEED_OPEN_ANGLE);
    delay(500);
    Serial.printf("🔧 SERVO AFTER: attached=%d, feeding=%d\n", 
                  feederServo.attached(), isFeeding);
    if (isScheduled) {
        Serial.printf("🍗 SCHEDULED FEEDING: Opening for %d seconds...\n", feedDurationSeconds);
    } else {
        Serial.println("🍗 MANUAL FEEDING: Opening dispenser...");
    }
}

void stopFeeding() {
    if (!isFeeding) {
        Serial.println("⚠️ FEEDING: Not currently feeding");
        return;
    }
    Serial.println("🍗 FEEDING: Closing dispenser.");
    if (feederServo.attached()) {
        feederServo.write(FEED_CLOSED_ANGLE);
        delay(500);
    } else {
        Serial.println("⚠️ SERVO NOT ATTACHED - reattaching to close");
        feederServo.attach(SERVO_PIN);
        delay(100);
        feederServo.write(FEED_CLOSED_ANGLE);
        delay(500);
    }
    isFeeding = false;
    isScheduledFeed = false;
}

// ==================== SCHEDULE CHECK FUNCTION ====================
void checkFeedingSchedule() {
    if (!timeSynchronized || !isFeedTimerAuto || isFeeding) {
        return;
    }
    DateTime now = rtc.now();
    static int lastCheckedHour = -1;
    static int lastCheckedMinute = -1;
    static bool fedThisMinute[4] = {false, false, false, false};
    int currentHour = now.hour();
    int currentMinute = now.minute();
    if (currentHour != lastCheckedHour || currentMinute != lastCheckedMinute) {
         for(int i=0; i<4; i++) { fedThisMinute[i] = false; }
         lastCheckedHour = currentHour;
         lastCheckedMinute = currentMinute;
    } else {
        return;
    }
    for (int i = 0; i < feedTimesCount; i++) {
        if (feedHours[i] != -1 && feedMinutes[i] != -1 &&
            feedHours[i] == currentHour && feedMinutes[i] == currentMinute &&
            !fedThisMinute[i])
        {
            Serial.printf("⏰ Feed Time %d matched (%02d:%02d)! Starting scheduled feed...\n", i + 1, currentHour, currentMinute);
            startFeeding(true);
            fedThisMinute[i] = true;
            break;
        }
    }
}

// ==================== SENSOR READING FUNCTIONS ====================
float getMq135Ppm() {
  const int MQ135_SAMPLES = 10;
  long sumAdc = 0;
  for (int i = 0; i < MQ135_SAMPLES; i++) {
    sumAdc += analogRead(MQ135_PIN);
    delay(2);  
  }
  float adcVal = (float)sumAdc / MQ135_SAMPLES;
  float vOut = adcVal * (V_IN / ADC_MAX_VAL);
  if (vOut <= 0) return 0;
  float Rs = RL_MQ135 * (V_IN - vOut) / vOut;
  float ratio = Rs / R0_MQ135;
  if (ratio <= 0) return 0;
  float ppmRaw = MQ135_a * pow(ratio, MQ135_b);
  static bool initialized = false;
  static float filteredPpm = 0.0;
  const float alpha = 0.15;   
  if (!initialized) {
    filteredPpm = ppmRaw;
    initialized = true;
  } else {
    filteredPpm = filteredPpm + alpha * (ppmRaw - filteredPpm);
  }
  const float MQ135_DISPLAY_SCALE = 1.1;   
  float ppmDisplay = filteredPpm * MQ135_DISPLAY_SCALE;
  return ppmDisplay;
}

float getUltrasonicDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 25000);
  float distance = (duration * 0.0343) / 2.0;
  if (distance == 0) return 999;
  return distance;
}