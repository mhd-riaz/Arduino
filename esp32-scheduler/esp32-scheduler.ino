// ============================================================================
// ESP32 Fish Tank Automation System (Arduino Sketch)
// Version: 2.0 - Optimized & Customized (5-Device System)
// Author: mhd-riaz
// Date: August 23, 2025
//
// Description:
// This program automates a fishtank environment using an ESP32 Dev Module.
// It controls five appliances (Filter, CO2, Light, Heater, HangOnFilter) based on 
// custom schedules, real-time temperature control, and user-initiated overrides via a REST API.
//
// Features:
// - Control of 5 appliances (Filter, CO2, Light, Heater, HangOnFilter).
// - Custom optimized schedules with minute-based timing for efficiency.
// - DS1307 RTC for accurate timekeeping, synchronized with NTP.
// - DS18B20 temperature sensor for intelligent heater control (25-29°C range).
// - 0.96" Blue OLED Display for real-time status (time, temperature, appliance states).
// - 12V 8-channel optocoupler relay for appliance control.
// - ESP32 WiFi for network connectivity with automatic fallback to AP mode.
// - REST API server with JSON input/output for schedule management,
//   temporary appliance control, and WiFi configuration.
// - Non-Volatile Storage (NVS) for WiFi credentials and appliance schedules.
// - Temperature-controlled heater logic with 30-minute minimum runtime.
// - Emergency safety system for extreme temperature conditions.
// - Memory optimized with reduced OLED update frequency and efficient scheduling.
// - Fault tolerance for WiFi network issues (RTC fallback, non-blocking ops).
// - Audio feedback system with buzzer for connection and emergency alerts.
// - Default REST API key: "Automate@123".
//
// Components Used:
// - ESP32 Dev Module (38-pin)
// - DS1307 RTC
// - DS18B20 temperature sensor
// - 0.96 Inch Blue OLED Display Module SPI/I2C - 4pin (SSD1306)
// - 12v 8 channel optocoupler relay
// - Buzzer
//
// Pinout (Refer to research document for detailed connections):
// - DS1307 RTC: SDA (GPIO 21), SCL (GPIO 22)
// - OLED Display: SDA (GPIO 21), SCL (GPIO 22)
// - DS18B20 Temp Sensor: DQ (GPIO 14) - Requires 4.7KΩ pull-up to 3.3V
// - Relay Module:
//   - IN1 (Filter): GPIO 16 - Main filtration system
//   - IN2 (CO2): GPIO 17 - CO2 injection system  
//   - IN3 (Light): GPIO 18 - Aquarium lighting
//   - IN4 (Heater): GPIO 19 - Water heater
//   - IN5 (HangOnFilter): GPIO 5 - Secondary hang-on filter
//   - Note: Relay type is configurable via RELAY_ACTIVE_LOW define.
// - Buzzer: GPIO 13 (PWM capable)
//
// Libraries Required:
// - WiFi (Built-in ESP32)
// - ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer)
// - AsyncTCP (https://github.com/me-no-dev/AsyncTCP) - Dependency for ESPAsyncWebServer
// - Preferences (Built-in ESP32)
// - Wire (Built-in Arduino)
// - RTClib (https://github.com/adafruit/RTClib)
// - OneWire (https://github.com/PaulStoffregen/OneWire)
// - DallasTemperature (https://github.com/milesburton/Arduino-Temperature-Control-Library)
// - Adafruit GFX Library (https://github.com/adafruit/Adafruit-GFX-Library)
// - Adafruit SSD1306 (https://github.com/adafruit/Adafruit_SSD1306)
// - ArduinoJson (https://github.com/bblanchon/ArduinoJson) - Version 6 recommended
// - NTPClient (https://github.com/arduino-libraries/NTPClient)
// - map (for std::map)
// - driver/ledc.h (for ledcAttach, ledcWriteTone) - ESP32 Arduino 3.0+ API
//
// Important Notes:
// - Ensure `Preferences.h` is used for NVS.
// - All timing operations use `millis()` for non-blocking behavior.
// - Schedules optimized to use minute-based timing (0-1439) for efficiency.
// - Relays are configurable via RELAY_ACTIVE_LOW define (true = Active LOW, false = Active HIGH).
// - Heater control prioritizes temperature over schedule/override with 30-min minimum runtime.
// - `ArduinoJson` v6 using JsonDocument for dynamic memory management.
// - Default custom schedules are loaded if NVS is empty.
// - Emergency safety system activates on extreme temperatures (>35°C or <10°C).
// - Memory optimized with reduced OLED update frequency (2-second intervals).
// - The system attempts to connect to saved WiFi. If unsuccessful, it
//   starts an Access Point (AP) "Fishtank_Setup" for configuration and provides audio alerts.
// - I2C pins (SDA, SCL) are shared between RTC and OLED.
// - A 4.7KΩ pull-up resistor is required for the DS18B20 data line to 3.3V.
// - For the relay module, use a separate 12V power supply for JD-VCC and
//   remove the VCC-JD-VCC jumper for opto-isolation.
// ============================================================================

// ============================================================================
// 1. Include Libraries
// ============================================================================
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Preferences.h>
#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <map>            // Required for std::map
#include <driver/ledc.h>  // Explicitly include for LEDC functions as per Espressif docs

// ============================================================================
// 1.5. Constants and Limits
// ============================================================================
#define MAX_POST_BODY_SIZE 2048  // Maximum size for POST request body
#define MAX_JSON_DOC_SIZE 4096   // Maximum size for JSON documents

// Override Constants
#define PERMANENT_OVERRIDE_VALUE ULONG_MAX  // Value used to indicate permanent override


// ============================================================================
// 2. Pin Definitions
// ============================================================================
// Appliance Relay Pins (Final GPIO assignments for 5-device system)
#define MOTOR_RELAY_PIN 16      // Main Filter (Primary filtration system)
#define CO2_RELAY_PIN 17        // CO2 System (CO2 injection for plants)
#define LIGHT_RELAY_PIN 18      // Aquarium Lights (LED lighting system)
#define HEATER_RELAY_PIN 19     // Water Heater (Temperature control)
#define HANGON_FILTER_PIN 5     // Hang-on Filter (Secondary filtration)

// Relay Configuration
// Set to true for Active LOW relays (LOW = ON, HIGH = OFF) - Most common
// Set to false for Active HIGH relays (HIGH = ON, LOW = OFF)
#define RELAY_ACTIVE_LOW false   // Change this based on your relay module type

// DS18B20 Temperature Sensor Pin
#define ONE_WIRE_BUS 14  // DS18B20 data pin (requires 4.7KΩ pull-up to 3.3V)

// Buzzer Pin (PWM capable for audio feedback)
#define BUZZER_PIN 13

// OLED Display (I2C - Shared with RTC on same bus)
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)
#define I2C_ADDRESS 0x3C  // Standard address for 128x64 SSD1306 displays

// ============================================================================
// 3. Global Constants and Variables
// ============================================================================

// API Server
const char *API_KEY = "Automate@123";
AsyncWebServer server(80);  // Web server on port 80

// NVS (Non-Volatile Storage)
Preferences preferences;
const char *NVS_NAMESPACE_WIFI = "wifi_creds";
const char *NVS_NAMESPACE_SCHEDULES = "app_schedules";
const char *NVS_KEY_SSID = "ssid";
const char *NVS_KEY_PASS = "password";
const char *NVS_KEY_SCHEDULES = "schedules_json";

// Time Management (NTP + RTC System)
RTC_DS1307 rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 5.5 * 3600, 60000);  // IST offset (5.5 hours), update every 60s
unsigned long lastNtpSyncMillis = 0;
const unsigned long NTP_SYNC_INTERVAL_MS = 3 * 3600 * 1000;  // Sync every 3 hours when WiFi available

// Temperature Sensor & Control System
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float currentTemperatureC = 0.0;
unsigned long lastTempReadMillis = 0;
const unsigned long TEMP_READ_INTERVAL_MS = 5000;  // Read temperature every 5 seconds

// Intelligent Heater Control Logic
const float TEMP_THRESHOLD_ON = 25.0;                         // Turn heater ON if temp drops below 25°C
const float TEMP_THRESHOLD_OFF = 29.0;                        // Turn heater OFF if temp reaches/exceeds 29°C
const unsigned long HEATER_MIN_RUN_TIME_MS = 30 * 60 * 1000;  // 30 minutes minimum continuous runtime
unsigned long heaterOnTimeMillis = 0;                         // Tracks when heater was turned ON for minimum runtime
bool heaterForcedOn = false;                                  // Flag indicating temperature-controlled heater state

// Appliance Management System
enum ApplianceState { OFF,
                      ON };
enum ApplianceMode { SCHEDULED,      // Following programmed schedule
                     OVERRIDDEN,     // Manual override active  
                     TEMP_CONTROLLED };  // Temperature-controlled (heater only)

struct ScheduleEntry {
  String type;    // "on_interval" or "off_interval"
  int start_min;  // Start time in minutes from midnight (0-1439) - Optimized format
  int end_min;    // End time in minutes from midnight (0-1439) - Optimized format
};

struct Appliance {
  String name;                        // Device name (Filter, CO2, Light, Heater, HangOnFilter)
  int pin;                           // GPIO pin for relay control
  ApplianceState currentState;       // Current physical state (ON/OFF)
  ApplianceState scheduledState;     // State according to schedule
  ApplianceState overrideState;      // State from manual override
  unsigned long overrideEndTime;     // millis() when override expires (0 = no override)
  ApplianceMode currentMode;         // Current control mode (SCHEDULED/OVERRIDDEN/TEMP_CONTROLLED)
};

// Array of 5 appliance objects with custom configurations
Appliance appliances[] = {
  { "Filter", MOTOR_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },           // Primary filtration (GPIO 16)
  { "CO2", CO2_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },                // CO2 injection system (GPIO 17)
  { "Light", LIGHT_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },            // LED lighting system (GPIO 18)
  { "Heater", HEATER_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },          // Water heater with temp control (GPIO 19)
  { "HangOnFilter", HANGON_FILTER_PIN, OFF, OFF, OFF, 0, SCHEDULED }    // Secondary hang-on filter (GPIO 20)
};
const int NUM_APPLIANCES = sizeof(appliances) / sizeof(appliances[0]);

// Map to store custom schedules for each appliance (minute-based for efficiency)
std::map<String, std::vector<ScheduleEntry>> applianceSchedules;

// Global POST body storage (single reusable buffer for memory optimization)
String postBody = "";

// Debug and logging configuration
#define DEBUG_MODE false  // Set to true for development, false for production to save memory
unsigned long lastDebugPrintMillis = 0;
const unsigned long DEBUG_PRINT_INTERVAL_MS = 30000;  // Print debug info every 30 seconds (reduced frequency)

// OLED Display System
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
unsigned long lastOLEDUpdateMillis = 0;
const unsigned long OLED_UPDATE_INTERVAL_MS = 2000;  // Update OLED every 2 seconds (memory optimized)

// WiFi Connection Management
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 30000;  // 30 second timeout for WiFi connection
bool wifiConnected = false;
bool apModeActive = false;

// Emergency Safety System  
bool emergencyShutdown = false;
const float EMERGENCY_TEMP_HIGH = 32.0;  // Emergency shutdown if temperature exceeds 32°C
const float EMERGENCY_TEMP_LOW = 20.0;   // Emergency shutdown if temperature below 20°C

// System Health Monitoring & Alert System
unsigned long lastSystemHeartbeat = 0;
unsigned long lastLoopTime = 0;
const unsigned long SYSTEM_WATCHDOG_TIMEOUT_MS = 60000;  // Alert if system doesn't respond for 60 seconds
const unsigned long LOOP_HANG_TIMEOUT_MS = 30000;       // Alert if main loop hangs for 30 seconds
bool systemHealthOK = true;
bool alertSounded = false;

// ============================================================================
// 4. Function Prototypes
// ============================================================================
// Core System Functions
void connectWiFi();
void startAPMode();
void buzz(int count, int delayMs);
void buzzPattern(int pattern);
void syncTimeNTP();

// System Health Monitoring
void updateSystemHeartbeat();
void checkSystemHealth();
void alertSystemFailure(const char* reason);

// Schedule Management
void loadSchedules();
void saveSchedules();
void applyApplianceLogic(Appliance &app, int currentMinutes);

// Relay Control Functions
void setRelayState(int pin, ApplianceState state);
int getRelayOffState();
int getRelayOnState();

// Display & Utility Functions  
void updateOLED(DateTime now);
bool isTimeInInterval(int currentMinutes, int startMin, int endMin);
bool authenticateRequest(AsyncWebServerRequest *request);

// Emergency & Safety System
void emergencyShutdownAll();

// REST API Handlers
void handleRoot(AsyncWebServerRequest *request);
void handleStatus(AsyncWebServerRequest *request);
void handleControl(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handleGetSchedules(AsyncWebServerRequest *request);
void handlePostSchedules(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handleWifiConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handleNotFound(AsyncWebServerRequest *request);
void handleEmergencyReset(AsyncWebServerRequest *request);
void handleResetToSchedule(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

// ============================================================================
// 4.5. Relay Control Helper Functions
// ============================================================================

/**
 * @brief Returns the digital state for relay OFF based on relay type
 * @return HIGH for active LOW relays, LOW for active HIGH relays
 */
int getRelayOffState() {
  return RELAY_ACTIVE_LOW ? HIGH : LOW;
}

/**
 * @brief Returns the digital state for relay ON based on relay type
 * @return LOW for active LOW relays, HIGH for active HIGH relays
 */
int getRelayOnState() {
  return RELAY_ACTIVE_LOW ? LOW : HIGH;
}

/**
 * @brief Sets relay state properly based on relay type configuration
 * @param pin GPIO pin connected to relay
 * @param state ApplianceState (ON or OFF)
 */
void setRelayState(int pin, ApplianceState state) {
  if (state == ON) {
    digitalWrite(pin, getRelayOnState());
  } else {
    digitalWrite(pin, getRelayOffState());
  }
}

// ============================================================================
// 5. Setup Function
// ============================================================================
void setup() {
  // Add small delay for power stabilization
  delay(500);
  
  Serial.begin(115200);
  Serial.println("\n[SETUP] Starting ESP32 Fish Tank Automation System...");
  
  // Add 2-second startup delay with beep indicator
  delay(2000);
  
  // Configure Buzzer Pin early for startup beep
  ledcAttach(BUZZER_PIN, 1000, 8);  // pin, frequency, resolution
  
  // Startup beep to indicate system is ready
  ledcWriteTone(BUZZER_PIN, 1500);  // 1.5kHz startup tone
  delay(300);                       // Beep duration
  ledcWriteTone(BUZZER_PIN, 0);     // Turn off
  #if DEBUG_MODE
  Serial.println("[SETUP] System startup beep completed.");
  #endif

  // Initialize NVS (WiFi namespace will be used initially)
  if (!preferences.begin(NVS_NAMESPACE_WIFI, false)) {
    Serial.println("[NVS] Failed to initialize WiFi preferences.");
  }

  // Initialize I2C bus
  Wire.begin(OLED_SDA, OLED_SCL);
  #if DEBUG_MODE
  Serial.println("[I2C] I2C bus initialized.");
  #endif

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("[ERROR] RTC not found!");
    // Sound continuous error buzzer to alert user
    while (1) {
      ledcWriteTone(BUZZER_PIN, 2000);  // 2kHz error tone
      delay(500);                       // Buzz for 500ms
      ledcWriteTone(BUZZER_PIN, 0);     // Turn off
      delay(1000);                      // Wait 1 second before next buzz
    }
  }
  if (!rtc.isrunning()) {
    #if DEBUG_MODE
    Serial.println("[RTC] RTC is NOT running, setting time from build time.");
    #endif
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set to compile time as fallback
  }
  #if DEBUG_MODE
  Serial.println("[RTC] RTC initialized.");
  #endif

  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
    #if DEBUG_MODE
    Serial.println("[ERROR] OLED init failed");
    #endif
  } else {
    #if DEBUG_MODE
    Serial.println("[OLED] Display initialized.");
    #endif
    display.display();  // Show initial Adafruit logo
    delay(2000);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Starting...");
    display.display();
  }

  // Initialize DS18B20 Temperature Sensor
  sensors.begin();
  #if DEBUG_MODE
  Serial.println("[DS18B20] Temperature sensor initialized.");
  #endif

  // Configure Relay Pins and ensure they are OFF initially
  for (int i = 0; i < NUM_APPLIANCES; i++) {
    pinMode(appliances[i].pin, OUTPUT);
    setRelayState(appliances[i].pin, OFF);  // Set to OFF using proper relay type
    #if DEBUG_MODE
    Serial.printf("[RELAY] Pin %d (%s) set to OFF (Relay Type: %s).\n", 
                  appliances[i].pin, appliances[i].name.c_str(), 
                  RELAY_ACTIVE_LOW ? "Active LOW" : "Active HIGH");
    #endif
  }

  // Attempt WiFi Connection
  connectWiFi();

  // If WiFi connection failed, start AP mode and buzz
  if (!wifiConnected) {
    buzz(3, 1000);  // Buzz thrice with 1000ms delay
    startAPMode();
  } else {
    // Sync RTC with NTP immediately after successful WiFi connection
    syncTimeNTP();
  }

  // Load schedules from NVS
  loadSchedules();

  // Apply initial appliance states based on current time and schedules
  DateTime now = rtc.now();
  int currentMinutes = now.hour() * 60 + now.minute();
  
  for (int i = 0; i < NUM_APPLIANCES; i++) {
    applyApplianceLogic(appliances[i], currentMinutes);
  }

  // Configure REST API Endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request){
    // Response will be sent by the body handler
  }, NULL, handleControl);
  server.on("/schedules", HTTP_GET, handleGetSchedules);
  server.on("/schedules", HTTP_POST, [](AsyncWebServerRequest *request){
    // Response will be sent by the body handler  
  }, NULL, handlePostSchedules);
  server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    // Response will be sent by the body handler
  }, NULL, handleWifiConfig);
  server.on("/emergency/reset", HTTP_POST, handleEmergencyReset);
  server.on("/reset", HTTP_POST, [](AsyncWebServerRequest *request){
    // Response will be sent by the body handler
  }, NULL, handleResetToSchedule);
  server.onNotFound(handleNotFound);

  // Start the server
  server.begin();
  #if DEBUG_MODE
  Serial.println("[API] REST API server started.");
  #endif
  Serial.println("[SETUP] System ready!");
}

// ============================================================================
// 6. Loop Function
// ============================================================================
void loop() {
  // Check emergency conditions (lightweight) - exclude zero values to prevent false alarms
  if (currentTemperatureC > 0.0 && (currentTemperatureC > EMERGENCY_TEMP_HIGH || currentTemperatureC < EMERGENCY_TEMP_LOW)) {
    if (!emergencyShutdown) emergencyShutdownAll();
  } else {
    // Auto-reset emergency if temperature returns to safe range and system was in emergency
    if (emergencyShutdown && currentTemperatureC > EMERGENCY_TEMP_LOW && currentTemperatureC < EMERGENCY_TEMP_HIGH && currentTemperatureC > 0.0) {
      emergencyShutdown = false;
      Serial.println("[EMERGENCY] Auto-reset: Temperature returned to safe range");
      buzz(2, 300);  // Confirmation buzzes
    }
  }

  // Handle WiFi reconnection if disconnected
  if (WiFi.status() != WL_CONNECTED && !apModeActive) {
    if (millis() - lastNtpSyncMillis > 5000) {  // Check every 5 seconds
      connectWiFi();
      lastNtpSyncMillis = millis();
    }
  }

  // Periodically sync RTC with NTP (every 3 hours, if WiFi available)
  if (wifiConnected && (millis() - lastNtpSyncMillis >= NTP_SYNC_INTERVAL_MS)) {
    syncTimeNTP();
    lastNtpSyncMillis = millis();
  }

  // Get current time from RTC
  DateTime now = rtc.now();
  int currentMinutes = now.hour() * 60 + now.minute();  // Convert to minutes once

  // Read temperature periodically
  if (millis() - lastTempReadMillis >= TEMP_READ_INTERVAL_MS) {
    sensors.requestTemperatures();
    float tempReading = sensors.getTempCByIndex(0);
    if (tempReading != DEVICE_DISCONNECTED_C && tempReading > -50.0 && tempReading < 100.0) {
      // Valid temperature reading - update current temperature
      currentTemperatureC = tempReading;
    }
    // If sensor disconnected or invalid reading, keep previous temperature value
    // This prevents false emergency triggers from sensor disconnection
    lastTempReadMillis = millis();
  }

  // Apply appliance control logic (unless emergency shutdown)
  if (!emergencyShutdown) {
    for (int i = 0; i < NUM_APPLIANCES; i++) {
      applyApplianceLogic(appliances[i], currentMinutes);
    }
  }

  // Conditional debug output (only if DEBUG_MODE enabled and reduced frequency)
  #if DEBUG_MODE
  if (millis() - lastDebugPrintMillis >= DEBUG_PRINT_INTERVAL_MS) {
    Serial.printf("[DEBUG] %02d:%02d T:%.1f°C Heap:%u Emergency:%s\n", 
                  now.hour(), now.minute(), currentTemperatureC, ESP.getFreeHeap(),
                  emergencyShutdown ? "Y" : "N");
    lastDebugPrintMillis = millis();
  }
  #endif

  // Update OLED display periodically (reduced frequency)
  if (millis() - lastOLEDUpdateMillis >= OLED_UPDATE_INTERVAL_MS) {
    updateOLED(now);
    lastOLEDUpdateMillis = millis();
  }
}

// ============================================================================
// 7. Helper Functions
// ============================================================================

/**
 * @brief Connects to a saved WiFi network or attempts to connect to a default.
 * If no credentials are saved, it will not connect.
 */
void connectWiFi() {
  // Switch to WiFi namespace to read credentials
  preferences.end();  // End current namespace
  if (!preferences.begin(NVS_NAMESPACE_WIFI, false)) {
    Serial.println("[NVS] Failed to open WiFi namespace.");
    wifiConnected = false;
    return;
  }
  
  String ssid = preferences.getString(NVS_KEY_SSID, "");
  String password = preferences.getString(NVS_KEY_PASS, "");

  if (ssid.length() == 0) {
    #if DEBUG_MODE
    Serial.println("[WiFi] No saved WiFi credentials. Cannot connect to STA mode.");
    #endif
    wifiConnected = false;
    return;
  }

  #if DEBUG_MODE
  Serial.printf("[WiFi] Attempting to connect to SSID: %s\n", ssid.c_str());
  #endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    #if DEBUG_MODE
    Serial.print(".");
    #endif
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Connecting WiFi...");
    display.setCursor(0, 10);
    display.println(ssid);
    display.display();
  }

  if (WiFi.status() == WL_CONNECTED) {
    #if DEBUG_MODE
    Serial.println("\n[WiFi] Connected to WiFi!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
    #endif
    wifiConnected = true;
    apModeActive = false;  // Ensure AP mode is off if STA connected
  } else {
    #if DEBUG_MODE
    Serial.println("\n[WiFi] Failed to connect to WiFi.");
    #endif
    wifiConnected = false;
  }
}

/**
 * @brief Starts the ESP32 in Access Point (AP) mode for configuration.
 */
void startAPMode() {
  #if DEBUG_MODE
  Serial.println("[AP] Starting Access Point mode...");
  #endif
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Fishtank_Setup", "password123");  // Default AP name and password
  IPAddress apIP = WiFi.softAPIP();
  #if DEBUG_MODE
  Serial.print("[AP] AP IP address: ");
  Serial.println(apIP);
  #endif
  apModeActive = true;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("AP Mode Active:");
  display.setCursor(0, 10);
  display.println("SSID: Fishtank_Setup");
  display.setCursor(0, 20);
  display.println("Pass: password123");
  display.setCursor(0, 30);
  display.print("IP: ");
  display.println(apIP);
  display.display();
}

/**
 * @brief Produces a buzzing sound from the buzzer.
 * @param count Number of buzzes.
 * @param delayMs Delay between buzzes in milliseconds.
 */
void buzz(int count, int delayMs) {
  #if DEBUG_MODE
  Serial.printf("[BUZZER] Buzzing %d times...\n", count);
  #endif
  for (int i = 0; i < count; i++) {
    ledcWriteTone(BUZZER_PIN, 1000);  // 1kHz tone
    delay(200);                       // Buzz duration
    ledcWriteTone(BUZZER_PIN, 0);     // Turn off
    if (i < count - 1) {
      delay(delayMs);
    }
  }
}

/**
 * @brief Synchronizes the DS1307 RTC with an NTP server.
 */
void syncTimeNTP() {
  if (WiFi.status() == WL_CONNECTED) {
    #if DEBUG_MODE
    Serial.println("[NTP] Syncing time with NTP server...");
    #endif
    timeClient.begin();
    if (timeClient.forceUpdate()) {
      unsigned long epochTime = timeClient.getEpochTime();
      rtc.adjust(DateTime(epochTime));
      #if DEBUG_MODE
      Serial.printf("[NTP] RTC adjusted to: %s\n", rtc.now().timestamp().c_str());
      #endif
      timeClient.end();  // End NTP client to free resources
    } else {
      #if DEBUG_MODE
      Serial.println("[NTP] Failed to get time from NTP server.");
      #endif
    }
  } else {
    #if DEBUG_MODE
    Serial.println("[NTP] WiFi not connected, skipping NTP sync.");
    #endif
  }
}

/**
 * @brief Loads appliance schedules from NVS. If no schedules are found,
 * custom default schedules optimized for fish tank automation are loaded and saved to NVS.
 * 
 * Default Schedule Summary:
 * - CO2: 8:30 AM - 1:30 PM (510-810), 3:30 PM - 8:30 PM (930-1230)
 * - Light: 9:30 AM - 1:30 PM (570-810), 4:30 PM - 8:30 PM (990-1230)  
 * - Heater: 12:00 AM - 4:30 AM (0-270), 8:30 PM - 11:59 PM (1230-1439)
 * - HangOnFilter: 6:30 AM - 8:30 AM (390-510), 8:30 PM - 10:30 PM (1230-1350)
 * - Filter: Continuous except 1:30 PM - 3:30 PM (810-930) for maintenance
 */
void loadSchedules() {
  // Switch to schedules namespace
  preferences.end();  // End current namespace
  if (!preferences.begin(NVS_NAMESPACE_SCHEDULES, false)) {
    Serial.println("[NVS] Failed to open schedules namespace.");
    return;
  }
  
  String schedulesJson = preferences.getString(NVS_KEY_SCHEDULES, "");

  if (schedulesJson.length() == 0) {
    #if DEBUG_MODE
    Serial.println("[NVS] No schedules found in NVS. Loading custom default schedules...");
    #endif
    // Define custom default schedules (all times converted to minutes for efficiency)
    // CO2: 8:30 AM - 1:30 PM (510-810), 3:30 PM - 8:30 PM (930-1230)
    applianceSchedules["CO2"].push_back({ "on_interval", 510, 810 });         
    applianceSchedules["CO2"].push_back({ "on_interval", 930, 1230 });        
    
    // Light: 9:30 AM - 1:30 PM (570-810), 4:30 PM - 8:30 PM (990-1230)
    applianceSchedules["Light"].push_back({ "on_interval", 570, 810 });       
    applianceSchedules["Light"].push_back({ "on_interval", 990, 1230 });      
    
    // Heater: 12:00 AM - 4:30 AM (0-270), 8:30 PM - 11:59 PM (1230-1439)
    applianceSchedules["Heater"].push_back({ "on_interval", 0, 270 });        
    applianceSchedules["Heater"].push_back({ "on_interval", 1230, 1439 });    
    
    // Hang-on Filter: 6:30 AM - 8:30 AM (390-510), 8:30 PM - 10:30 PM (1230-1350)
    applianceSchedules["HangOnFilter"].push_back({ "on_interval", 390, 510 }); 
    applianceSchedules["HangOnFilter"].push_back({ "on_interval", 1230, 1350 });
    
    // Filter: 1:30 PM - 3:30 PM (810-930) - OFF during this time for maintenance
    applianceSchedules["Filter"].push_back({ "off_interval", 810, 930 });     

    saveSchedules();
    #if DEBUG_MODE
    Serial.println("[NVS] Default schedules loaded and saved:");
    Serial.println("  CO2: 8:30-13:30 (510-810), 15:30-20:30 (930-1230)");
    Serial.println("  Light: 9:30-13:30 (570-810), 16:30-20:30 (990-1230)");
    Serial.println("  Heater: 0:00-4:30 (0-270), 20:30-23:59 (1230-1439)");
    Serial.println("  HangOnFilter: 6:30-8:30 (390-510), 20:30-22:30 (1230-1350)");
    Serial.println("  Filter: OFF 13:30-15:30 (810-930), otherwise ON");
    #endif
    return;
  }

  #if DEBUG_MODE
  Serial.println("[NVS] Loading schedules from NVS...");
  #endif
  JsonDocument doc;  // Changed from StaticJsonDocument to JsonDocument
  DeserializationError error = deserializeJson(doc, schedulesJson);

  if (error) {
    #if DEBUG_MODE
    Serial.print(F("[NVS] deserializeJson() failed: "));
    Serial.println(error.f_str());
    #endif
    return;
  }

  applianceSchedules.clear();  // Clear existing schedules before loading new ones

  for (JsonPair p : doc.as<JsonObject>()) {
    String applianceName = p.key().c_str();
    JsonArray schedulesArray = p.value().as<JsonArray>();

    for (JsonObject scheduleObj : schedulesArray) {
      ScheduleEntry entry;
      entry.type = scheduleObj["type"].as<String>();
      // Convert hours and minutes to total minutes, or use direct minutes if provided
      if (scheduleObj.containsKey("start_min")) {
        entry.start_min = scheduleObj["start_min"].as<int>();
        entry.end_min = scheduleObj["end_min"].as<int>();
      } else {
        // Legacy format conversion
        entry.start_min = scheduleObj["start_h"].as<int>() * 60 + scheduleObj["start_m"].as<int>();
        entry.end_min = scheduleObj["end_h"].as<int>() * 60 + scheduleObj["end_m"].as<int>();
      }
      applianceSchedules[applianceName].push_back(entry);
    }
  }
  #if DEBUG_MODE
  Serial.println("[NVS] Schedules loaded successfully.");
  #endif
}

/**
 * @brief Saves current appliance schedules to NVS.
 */
void saveSchedules() {
  // Switch to schedules namespace
  preferences.end();  // End current namespace
  if (!preferences.begin(NVS_NAMESPACE_SCHEDULES, false)) {
    Serial.println("[NVS] Failed to open schedules namespace for saving.");
    return;
  }
  
  JsonDocument doc;  // Changed from StaticJsonDocument to JsonDocument

  for (auto const &[applianceName, schedulesVec] : applianceSchedules) {
    // Use doc[key].to<JsonArray>() for modern ArduinoJson
    JsonArray schedulesArray = doc[applianceName].to<JsonArray>();
    for (const auto &entry : schedulesVec) {
      JsonObject scheduleObj = schedulesArray.add<JsonObject>();
      scheduleObj["type"] = entry.type;
      scheduleObj["start_min"] = entry.start_min;
      scheduleObj["end_min"] = entry.end_min;
    }
  }

  String output;
  serializeJson(doc, output);
  preferences.putString(NVS_KEY_SCHEDULES, output);
  #if DEBUG_MODE
  Serial.println("[NVS] Schedules saved to NVS.");
  Serial.println(output);  // For debugging
  #endif
}

/**
 * @brief Applies the intelligent control logic for a single appliance, considering custom schedules,
 * manual overrides, and temperature-based control (for heater).
 * Priority order: Temperature Control > Manual Override > Scheduled Control
 * @param app Reference to the Appliance object.
 * @param currentMinutes Current time in minutes from midnight (0-1439).
 */
void applyApplianceLogic(Appliance &app, int currentMinutes) {
  ApplianceState targetState = OFF;
  app.currentMode = SCHEDULED;

  // 1. Check for active override
  if (app.overrideEndTime > 0) {
    if (app.overrideEndTime == PERMANENT_OVERRIDE_VALUE || (long)(millis() - app.overrideEndTime) < 0) {  // Permanent or active temporary override
      targetState = app.overrideState;
      app.currentMode = OVERRIDDEN;
    } else {
      app.overrideEndTime = 0;  // Override expired
    }
  }

  // 2. Apply Scheduled Logic (only if not overridden)
  if (app.overrideEndTime == 0) {
    // Default behavior: Filter is ON by default, others are OFF by default
    bool defaultState = (app.name == "Filter");
    targetState = defaultState ? ON : OFF;
    
    if (applianceSchedules.count(app.name)) {
      std::vector<ScheduleEntry> &schedules = applianceSchedules[app.name];
      bool isScheduledOn = false;
      bool isScheduledOff = false;

      for (const auto &schedule : schedules) {
        if (isTimeInInterval(currentMinutes, schedule.start_min, schedule.end_min)) {
          if (schedule.type == "on_interval") {
            isScheduledOn = true;
          } else if (schedule.type == "off_interval") {
            isScheduledOff = true;
          }
        }
      }

      // Apply schedule logic based on appliance type
      if (app.name == "Filter") {
        // Filter: ON by default, turn OFF only during off_interval
        targetState = isScheduledOff ? OFF : ON;
      } else {
        // Other appliances: OFF by default, turn ON only during on_interval  
        targetState = isScheduledOn ? ON : OFF;
      }
    }
    app.scheduledState = targetState;
  }

  // 3. Intelligent Heater Temperature Logic (highest priority - overrides everything)
  if (app.name == "Heater" && currentTemperatureC > 0) {
    if (currentTemperatureC < TEMP_THRESHOLD_ON) {
      // Temperature too low - force heater ON
      if (app.currentState == OFF && !heaterForcedOn) {
        heaterOnTimeMillis = millis();
        heaterForcedOn = true;
      }
      targetState = ON;
      app.currentMode = TEMP_CONTROLLED;
    } else if (currentTemperatureC >= TEMP_THRESHOLD_OFF) {
      // Temperature reached target - check minimum runtime
      if (heaterForcedOn && ((long)(millis() - heaterOnTimeMillis) < HEATER_MIN_RUN_TIME_MS)) {  // Fix overflow
        targetState = ON;  // Keep ON for minimum 30-minute runtime
        app.currentMode = TEMP_CONTROLLED;
      } else {
        heaterForcedOn = false;
        heaterOnTimeMillis = 0;
        targetState = OFF;
        app.currentMode = (app.overrideEndTime > 0) ? OVERRIDDEN : SCHEDULED;
      }
    }
    // Ensure minimum runtime is respected
    if (heaterForcedOn && targetState == OFF) {
      targetState = ON;
      app.currentMode = TEMP_CONTROLLED;
    }
  }

  // Update appliance state if changed
  if (app.currentState != targetState) {
    app.currentState = targetState;
    setRelayState(app.pin, app.currentState);  // Use configurable relay control
    // Only log state changes for critical events (temperature control) or if debug mode
    #if DEBUG_MODE
    Serial.printf("[CONTROL] %s: %s\n", app.name.c_str(), (app.currentState == ON ? "ON" : "OFF"));
    #else
    if (app.name == "Heater" && app.currentMode == TEMP_CONTROLLED) {
      Serial.printf("[TEMP] Heater: %s\n", (app.currentState == ON ? "ON" : "OFF"));
    }
    #endif
  }
}

/**
 * @brief Updates the OLED display with current system status including time, 
 * temperature, appliance states, and connection status.
 * Display optimized for 128x64 SSD1306 with 2-second update intervals for memory efficiency.
 * @param now Current DateTime from RTC.
 */
void updateOLED(DateTime now) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Row 1: Current Time
  display.setCursor(0, 0);
  display.printf("%02d:%02d:%02d %02d/%02d", now.hour(), now.minute(), now.second(), now.day(), now.month());

  // Row 2 onwards: Appliance States with compact formatting
  int y = 10;
  for (int i = 0; i < NUM_APPLIANCES; i++) {
    display.setCursor(0, y + (i * 8));
    display.printf("%s: %s", appliances[i].name.c_str(), (appliances[i].currentState == ON ? "ON" : "OFF"));
  }

  // Temperature and System Status (bottom row)
  display.setCursor(0, y + (NUM_APPLIANCES * 8));
  display.printf("%.1fC", currentTemperatureC);
  
  display.setCursor(40, y + (NUM_APPLIANCES * 8));
  if (emergencyShutdown) {
    display.print("EMERGENCY!");
  } else if (WiFi.status() == WL_CONNECTED) {
    // Display IP address instead of "WiFi: OK" for easier API access
    display.print(WiFi.localIP());
  } else if (apModeActive) {
    display.print("AP Mode");
  } else {
    display.print("No WiFi");
  }

  display.display();
}

/**
 * @brief Checks if a given time falls within a specified interval using minute-based comparison.
 * Efficiently handles both normal intervals (7:00-14:00) and overnight intervals (22:00-06:00).
 * @param currentMinutes Current time in minutes from midnight (0-1439).
 * @param startMin Start of interval in minutes from midnight (0-1439).
 * @param endMin End of interval in minutes from midnight (0-1439).
 * @return True if current time falls within the interval, false otherwise.
 */
bool isTimeInInterval(int currentMinutes, int startMin, int endMin) {
  if (startMin <= endMin) {
    // Normal interval (e.g., 420-840 = 7:00-14:00)
    return (currentMinutes >= startMin && currentMinutes < endMin);
  } else {
    // Overnight interval (e.g., 1320-360 = 22:00-06:00)
    return (currentMinutes >= startMin || currentMinutes < endMin);
  }
}

/**
 * @brief Authenticates incoming API requests using the API_KEY.
 * @param request Pointer to the AsyncWebServerRequest object.
 * @return True if authenticated, false otherwise.
 */
bool authenticateRequest(AsyncWebServerRequest *request) {
  if (request->hasHeader("X-API-Key") && request->header("X-API-Key") == API_KEY) {
    return true;
  }
  request->send(401, "application/json", "{\"error\": \"Unauthorized\", \"message\": \"Missing or invalid X-API-Key header.\"}\n");
  return false;
}

// ============================================================================
// 8. REST API Handlers
// ============================================================================

/**
 * @brief Handles the root ("/") endpoint.
 */
void handleRoot(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", "ESP32 Fish Tank Automation System API. Use /status, /control, /schedules, /wifi.");
}

/**
 * @brief Handles the "/status" GET endpoint. Returns current system status.
 */
void handleStatus(AsyncWebServerRequest *request) {
  if (!authenticateRequest(request)) return;

  JsonDocument doc;  // Changed from StaticJsonDocument to JsonDocument
  DateTime now = rtc.now();

  doc["timestamp"] = now.timestamp();
  doc["temperature_celsius"] = currentTemperatureC;

  // Use doc[key].to<JsonObject>() for modern ArduinoJson
  JsonObject appliances_json = doc["appliances"].to<JsonObject>();
  for (int i = 0; i < NUM_APPLIANCES; i++) {
    JsonObject app_status = appliances_json[appliances[i].name].to<JsonObject>();
    app_status["state"] = (appliances[i].currentState == ON ? "ON" : "OFF");
    String modeStr;
    switch (appliances[i].currentMode) {
      case SCHEDULED: modeStr = "SCHEDULED"; break;
      case OVERRIDDEN: modeStr = "OVERRIDDEN"; break;
      case TEMP_CONTROLLED: modeStr = "TEMP_CONTROLLED"; break;
    }
    app_status["mode"] = modeStr;
    if (appliances[i].overrideEndTime > 0 && appliances[i].currentMode == OVERRIDDEN) {
      if (appliances[i].overrideEndTime == PERMANENT_OVERRIDE_VALUE) {
        app_status["override_type"] = "permanent";
      } else {
        app_status["override_type"] = "temporary";
        app_status["timeout_seconds"] = (appliances[i].overrideEndTime - millis()) / 1000;
      }
    }
  }

  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

/**
 * @brief Handles the "/control" POST endpoint. Allows overriding appliance states.
 * JSON Input (Consistent Array Format):
 * {
 * "appliances": [
 * {"name": "CO2", "action": "OFF", "timeout_minutes": 30},
 * {"name": "Light", "action": "ON"},
 * {"name": "Filter", "action": "ON", "timeout_minutes": 60}
 * ]
 * }
 * 
 * Single appliance example:
 * {
 * "appliances": [
 * {"name": "Heater", "action": "ON", "timeout_minutes": 120}
 * ]
 * }
 */
void handleControl(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (!authenticateRequest(request)) return;

  // Check total size limit
  if (total > MAX_POST_BODY_SIZE) {
    request->send(413, "application/json", "{\"error\": \"Request body too large\"}\n");
    return;
  }

  // Store the POST body data
  if (index == 0) {
    postBody = "";
    postBody.reserve(total + 1);  // Reserve memory
  }
  for (size_t i = 0; i < len; i++) {
    postBody += (char)data[i];
  }

  // Only process when we have received all the data
  if (index + len == total) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, postBody);

    if (error) {
      Serial.print(F("[API] JSON parse failed: "));
      Serial.println(error.f_str());
      request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}\n");
      return;
    }

    JsonArray appliancesArray = doc["appliances"].as<JsonArray>();

    if (appliancesArray.isNull()) {
      request->send(400, "application/json", "{\"error\": \"Missing 'appliances' array in JSON body\"}\n");
      return;
    }

    // Process appliances array
    for (JsonObject appObj : appliancesArray) {
      String appName = appObj["name"].as<String>();
      String action = appObj["action"].as<String>();
      int timeoutMinutes = appObj["timeout_minutes"] | 0;

      bool found = false;
      for (int i = 0; i < NUM_APPLIANCES; i++) {
        if (appliances[i].name == appName) {
          found = true;
          appliances[i].overrideState = (action == "ON" ? ON : OFF);
          // Prevent overflow: limit timeout to ~35 days (50000 minutes)
          if (timeoutMinutes > 50000) timeoutMinutes = 50000;
          // Use PERMANENT_OVERRIDE_VALUE for permanent override, calculated time for temporary override
          appliances[i].overrideEndTime = (timeoutMinutes > 0) ? (millis() + (unsigned long)timeoutMinutes * 60UL * 1000UL) : PERMANENT_OVERRIDE_VALUE;
          break;
        }
      }
      if (!found) {
        request->send(404, "application/json", "{\"error\": \"Appliance not found: " + appName + "\"}\n");
        return;
      }
    }
    request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Appliance control updated\"}\n");
  }
}

/**
 * @brief Handles the "/schedules" GET endpoint. Returns all stored schedules.
 */
void handleGetSchedules(AsyncWebServerRequest *request) {
  if (!authenticateRequest(request)) return;

  JsonDocument doc;  // Changed from StaticJsonDocument to JsonDocument
  // Use doc[key].to<JsonObject>() for modern ArduinoJson
  JsonObject schedules_json = doc["schedules"].to<JsonObject>();

  for (auto const &[applianceName, schedulesVec] : applianceSchedules) {
    // Use obj[key].to<JsonArray>() for modern ArduinoJson
    JsonArray schedulesArray = schedules_json[applianceName].to<JsonArray>();
    for (const auto &entry : schedulesVec) {
      JsonObject scheduleObj = schedulesArray.add<JsonObject>();
      scheduleObj["type"] = entry.type;
      scheduleObj["start_min"] = entry.start_min;
      scheduleObj["end_min"] = entry.end_min;
    }
  }

  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

/**
 * @brief Handles the "/schedules" POST endpoint. Allows updating all schedules.
 * JSON Input:
 * {
 * "api_key": "Automate@123", // Provided in header, but can also be in body for testing
 * "schedules": {
 * "Motor": [
 * {"type": "off_interval", "start_h": 9, "start_m": 30, "end_h": 10, "end_m": 30}
 * ],
 * "CO2": [
 * {"type": "on_interval", "start_h": 7, "start_m": 0, "end_h": 14, "end_m": 0},
 * {"type": "on_interval", "start_h": 15, "start_m": 0, "end_h": 21, "end_m": 0}
 * ],
 * // ... other appliances
 * }
 * }
 */
void handlePostSchedules(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (!authenticateRequest(request)) return;

  // Check total size limit
  if (total > MAX_POST_BODY_SIZE) {
    request->send(413, "application/json", "{\"error\": \"Request body too large\"}\n");
    return;
  }

  // Store the POST body data
  if (index == 0) {
    postBody = "";
    postBody.reserve(total + 1);  // Reserve memory
  }
  for (size_t i = 0; i < len; i++) {
    postBody += (char)data[i];
  }

  // Only process when we have received all the data
  if (index + len == total) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, postBody);

    if (error) {
      Serial.print(F("[API] deserializeJson() failed for /schedules: "));
      Serial.println(error.f_str());
      Serial.print(F("[API] Received body: "));
      Serial.println(postBody);
      request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}\n");
      return;
    }

    JsonObject incomingSchedules = doc["schedules"].as<JsonObject>();
    if (incomingSchedules.isNull()) {
      request->send(400, "application/json", "{\"error\": \"Missing 'schedules' object in JSON body\"}\n");
      return;
    }

    applianceSchedules.clear();  // Clear all existing schedules

    for (JsonPair p : incomingSchedules) {
      String applianceName = p.key().c_str();
      JsonArray schedulesArray = p.value().as<JsonArray>();

      for (JsonObject scheduleObj : schedulesArray) {
        ScheduleEntry entry;
        entry.type = scheduleObj["type"].as<String>();
        // Convert hours and minutes to total minutes, or use direct minutes if provided
        if (scheduleObj.containsKey("start_min")) {
          entry.start_min = scheduleObj["start_min"].as<int>();
          entry.end_min = scheduleObj["end_min"].as<int>();
        } else {
          // Legacy format conversion
          entry.start_min = scheduleObj["start_h"].as<int>() * 60 + scheduleObj["start_m"].as<int>();
          entry.end_min = scheduleObj["end_h"].as<int>() * 60 + scheduleObj["end_m"].as<int>();
        }
        applianceSchedules[applianceName].push_back(entry);
      }
    }

    saveSchedules();  // Save the new schedules to NVS
    request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Schedules updated and saved to NVS\"}\n");
  }
}

/**
 * @brief Handles the "/wifi" POST endpoint. Allows updating WiFi credentials.
 * JSON Input:
 * {
 * "api_key": "Automate@123", // Provided in header, but can also be in body for testing
 * "ssid": "MyNewHomeNetwork",
 * "password": "MyNewWifiPassword"
 * }
 */
void handleWifiConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (!authenticateRequest(request)) return;

  // Check total size limit  
  if (total > MAX_POST_BODY_SIZE) {
    request->send(413, "application/json", "{\"error\": \"Request body too large\"}\n");
    return;
  }

  // Store the POST body data
  if (index == 0) {
    postBody = "";
    postBody.reserve(total + 1);  // Reserve memory
  }
  for (size_t i = 0; i < len; i++) {
    postBody += (char)data[i];
  }

  // Only process when we have received all the data
  if (index + len == total) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, postBody);

    if (error) {
      Serial.print(F("[API] deserializeJson() failed for /wifi: "));
      Serial.println(error.f_str());
      Serial.print(F("[API] Received body: "));
      Serial.println(postBody);
      request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}\n");
      return;
    }

    String newSsid = doc["ssid"].as<String>();
    String newPassword = doc["password"].as<String>();

    if (newSsid.length() > 0) {
      // Switch to WiFi namespace for saving credentials
      preferences.end();  // End current namespace
      if (preferences.begin(NVS_NAMESPACE_WIFI, false)) {
        preferences.putString(NVS_KEY_SSID, newSsid);
        preferences.putString(NVS_KEY_PASS, newPassword);
        preferences.end();  // Close namespace before reboot
        Serial.printf("[NVS] New WiFi credentials saved: SSID=%s\n", newSsid.c_str());
        request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"WiFi credentials saved. Rebooting to connect...\"}\n");
        delay(1000);    // Give time for response to send
        ESP.restart();  // Soft reboot to connect to new WiFi
      } else {
        request->send(500, "application/json", "{\"error\": \"Failed to save WiFi credentials\"}\n");
      }
    } else {
      request->send(400, "application/json", "{\"error\": \"SSID cannot be empty\"}\n");
    }
  }
}

/**
 * @brief Handles requests for unknown endpoints.
 */
void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not Found");
}

// ============================================================================
// 9. System Health and Safety Functions  
// ============================================================================

/**
 * @brief Emergency shutdown of all appliances
 */
void emergencyShutdownAll() {
  Serial.println("[EMERGENCY] Emergency shutdown!");
  emergencyShutdown = true;
  
  // Turn off all appliances immediately using proper relay control
  for (int i = 0; i < NUM_APPLIANCES; i++) {
    appliances[i].currentState = OFF;
    setRelayState(appliances[i].pin, OFF);  // Use configurable relay control
  }
  
  // Sound emergency alert
  for (int i = 0; i < 5; i++) {
    ledcWriteTone(BUZZER_PIN, 2000);
    delay(200);
    ledcWriteTone(BUZZER_PIN, 0);
    delay(200);
  }
}

/**
 * @brief Handles the "/emergency/reset" POST endpoint. Resets emergency shutdown.
 */
void handleEmergencyReset(AsyncWebServerRequest *request) {
  if (!authenticateRequest(request)) return;

  if (!emergencyShutdown) {
    request->send(400, "application/json", "{\"error\": \"No emergency shutdown active\"}\n");
    return;
  }
  
  // Check if conditions are safe to reset
  if (currentTemperatureC > EMERGENCY_TEMP_LOW && currentTemperatureC < EMERGENCY_TEMP_HIGH) {
    emergencyShutdown = false;
    Serial.println("[EMERGENCY] Emergency shutdown reset");
    request->send(200, "application/json", "{\"status\": \"success\"}\n");
    buzz(1, 500);  // Confirmation buzz
  } else {
    request->send(400, "application/json", "{\"error\": \"Unsafe conditions persist\"}\n");
  }
}

/**
 * @brief Handles the "/reset" POST endpoint. Resets appliance overrides back to scheduled behavior.
 * JSON Input:
 * {
 * "appliances": ["Light", "CO2"]  // Optional: specific appliances to reset
 * }
 * 
 * Or reset all appliances:
 * {
 * "reset_all": true
 * }
 */
void handleResetToSchedule(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (!authenticateRequest(request)) return;

  // Check total size limit
  if (total > MAX_POST_BODY_SIZE) {
    request->send(413, "application/json", "{\"error\": \"Request body too large\"}\n");
    return;
  }

  // Store the POST body data
  if (index == 0) {
    postBody = "";
    postBody.reserve(total + 1);  // Reserve memory
  }
  for (size_t i = 0; i < len; i++) {
    postBody += (char)data[i];
  }

  // Only process when we have received all the data
  if (index + len == total) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, postBody);

    if (error) {
      Serial.print(F("[API] JSON parse failed for /reset: "));
      Serial.println(error.f_str());
      request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}\n");
      return;
    }

    bool resetAll = doc["reset_all"] | false;
    JsonArray appliancesToReset = doc["appliances"].as<JsonArray>();

    int resetCount = 0;

    if (resetAll) {
      // Reset all appliances
      for (int i = 0; i < NUM_APPLIANCES; i++) {
        appliances[i].overrideEndTime = 0;  // Clear override
        appliances[i].overrideState = OFF;   // Reset override state
        resetCount++;
      }
      Serial.println("[API] All appliance overrides reset to schedule");
    } else if (!appliancesToReset.isNull()) {
      // Reset specific appliances
      for (JsonVariant applianceName : appliancesToReset) {
        String appName = applianceName.as<String>();
        
        bool found = false;
        for (int i = 0; i < NUM_APPLIANCES; i++) {
          if (appliances[i].name == appName) {
            appliances[i].overrideEndTime = 0;  // Clear override
            appliances[i].overrideState = OFF;   // Reset override state
            found = true;
            resetCount++;
            break;
          }
        }
        if (!found) {
          request->send(404, "application/json", "{\"error\": \"Appliance not found: " + appName + "\"}\n");
          return;
        }
      }
      Serial.printf("[API] %d appliance overrides reset to schedule\n", resetCount);
    } else {
      request->send(400, "application/json", "{\"error\": \"Must specify either 'reset_all': true or 'appliances' array\"}\n");
      return;
    }

    request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"" + String(resetCount) + " appliances reset to schedule\"}\n");
  }
}
