// ============================================================================
// ESP32 Fish Tank Automation System (Arduino Sketch)
// Version: 3.2.0 - Safe GPIO Pin Reassignment
// Author: mhd-riaz
// Date: November 2, 2025
//
// See CHANGELOG.md for version history and updates.
// ============================================================================

#define ENABLE_OLED 1  // Set to 0 to disable OLED completely (saves ~30mA)

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
#include <driver/ledc.h>  // Include for both ESP32 Core 2.x and 3.0+ compatibility

// ============================================================================
// 1.4. ESP32 Arduino Core Compatibility Layer
// ============================================================================
// Cross-compatibility for ESP32 Arduino Core 2.x and 3.0+
// Core 3.0+ deprecated ledcAttach/ledcWriteTone in favor of simpler APIs

#if ESP_ARDUINO_VERSION_MAJOR >= 3
// ESP32 Arduino Core 3.0+ - Use modern tone() API
#define BUZZER_INIT(pin, freq, resolution)  // No initialization needed for tone()
#define BUZZER_TONE(pin, frequency) tone(pin, frequency)
#define BUZZER_OFF(pin) noTone(pin)
#else
// ESP32 Arduino Core 2.x - Use legacy LEDC API (channel 0)
#define BUZZER_INIT(pin, freq, resolution) \
  ledcAttachPin(pin, 0); \
  ledcSetup(0, freq, resolution)
#define BUZZER_TONE(pin, frequency) ledcWriteTone(0, frequency)
#define BUZZER_OFF(pin) ledcWriteTone(0, 0)
#endif

// ============================================================================
// 1.5. Constants and Limits (Optimized)
// ============================================================================
#define MAX_POST_BODY_SIZE 1536   // Reduced from 2048 for better memory usage
#define MAX_JSON_DOC_SIZE 2048    // Reduced from 4096 for better memory usage
#define RESPONSE_BUFFER_SIZE 512  // Buffer for HTTP responses

// Override Constants
#define PERMANENT_OVERRIDE_VALUE ULONG_MAX  // Value used to indicate permanent override

// ============================================================================
// 1.6. PROGMEM String Literals (Memory Optimization)
// ============================================================================
// Store frequently used strings in flash memory to save RAM
const char PROGMEM STR_OK[] = "OK";
const char PROGMEM STR_ERROR[] = "Error";
const char PROGMEM STR_SUCCESS[] = "Success";
const char PROGMEM STR_INVALID[] = "Invalid";
const char PROGMEM STR_JSON_PARSE_ERROR[] = "JSON parse error";
const char PROGMEM STR_MISSING_PARAMS[] = "Missing required parameters";
const char PROGMEM STR_INVALID_API_KEY[] = "Invalid API key";
const char PROGMEM STR_APPLIANCE_NOT_FOUND[] = "Appliance not found";
const char PROGMEM STR_CONTENT_TYPE_JSON[] = "application/json";
const char PROGMEM STR_ACCESS_CONTROL[] = "Access-Control-Allow-Origin";
const char PROGMEM STR_WILDCARD[] = "*";


// ============================================================================
// 2. Pin Definitions
// ============================================================================
// Appliance Relay Pins (Safe GPIO assignments - WiFi compatible, no strapping pins)
// Avoids strapping pins (0,2,5,12,15) and ADC2 conflicts with WiFi
#define MOTOR_RELAY_PIN 16    // Main Filter (Primary filtration system) - GPIO 16
#define CO2_RELAY_PIN 17      // CO2 System (CO2 injection for plants) - GPIO 17
#define LIGHT_RELAY_PIN 18    // Aquarium Lights (LED lighting system) - GPIO 18
#define HEATER_RELAY_PIN 19   // Water Heater (Temperature control) - GPIO 19
#define HANGON_FILTER_PIN 23  // Hang-on Filter (Secondary filtration) - GPIO 23
#define WAVE_MAKER_PIN 32     // Wave Maker (Water movement) - GPIO 32 (ADC1, safe with WiFi)

// Relay Configuration
// Set to true for Active LOW relays (LOW = ON, HIGH = OFF) - Most common
// Set to false for Active HIGH relays (HIGH = ON, LOW = OFF)
#define RELAY_ACTIVE_LOW false  // Professional optocoupler modules are Active LOW

// Debug and logging configuration
#define DEBUG_MODE true  // Set to true for development, false for production to save memory

// DS18B20 Temperature Sensor Pin
#define ONE_WIRE_BUS 33  // DS18B20 data pin (requires 4.7KΩ pull-up to 3.3V) - GPIO 33 (ADC1, safe)

// Buzzer Pin (PWM capable for audio feedback)
#define BUZZER_PIN 25    // GPIO 25 - Safe for PWM output

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
RTC_DS3231 rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 5.5 * 3600, 60000);  // IST offset (5.5 hours), update every 60s
unsigned long lastNtpSyncMillis = 0;
const unsigned long NTP_SYNC_INTERVAL_MS = 3 * 3600 * 1000;  // Sync every 3 hours when WiFi available

// Temperature Sensor & Control System
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float currentTemperatureC = -127.0;  // Initialize with sensor error value
unsigned long lastTempReadMillis = 0;
const unsigned long TEMP_READ_INTERVAL_MS = 15000;       // Normal: 15 seconds
const unsigned long TEMP_READ_ERROR_INTERVAL_MS = 5000;  // Error: 5 seconds

// DS18B20 Sensor Error Handling
bool tempSensorError = false;
unsigned long lastTempSensorErrorBuzzMillis = 0;
const unsigned long TEMP_SENSOR_ERROR_BUZZ_INTERVAL_MS = 3600000UL;  // 1 hour

// Intelligent Heater Control Logic
const float TEMP_THRESHOLD_ON = 25.0;                         // Turn heater ON if temp drops below 25°C
const float TEMP_THRESHOLD_OFF = 29.0;                        // Turn heater OFF if temp reaches/exceeds 29°C
const unsigned long HEATER_MIN_RUN_TIME_MS = 30 * 60 * 1000;  // 30 minutes minimum continuous runtime
unsigned long heaterOnTimeMillis = 0;                         // Tracks when heater was turned ON for minimum runtime
bool heaterForcedOn = false;                                  // Flag indicating temperature-controlled heater state
bool lastHeaterState = false;                                 // Cache last heater state to avoid redundant operations
int temperatureReadFailures = 0;                              // Track consecutive temperature read failures
const int MAX_TEMP_FAILURES = 5;                              // Reduced from 8 for memory optimization
int temperatureRecoveryCount = 0;                             // Track successful readings after failures
const int MIN_RECOVERY_READINGS = 3;                          // Require 3 good readings to clear emergency

// RTC Status Tracking
bool rtcInitialized = false;  // Track RTC initialization status

// OLED Display Status Tracking
bool oledInitialized = false;  // Track OLED initialization status

// Critical Section Protection
bool heaterControlLock = false;  // Simple lock for heater control operations

// Appliance Management System (Optimized)
enum ApplianceState { OFF,
                      ON };
enum ApplianceMode { SCHEDULED,          // Following programmed schedule
                     OVERRIDDEN,         // Manual override active
                     TEMP_CONTROLLED };  // Temperature-controlled (heater only)

struct ScheduleEntry {
  String type;    // "on_interval" or "off_interval"
  int start_min;  // Start time in minutes from midnight (0-1439) - Optimized format
  int end_min;    // End time in minutes from midnight (0-1439) - Optimized format
};

struct Appliance {
  String name;                    // Device name (Filter, CO2, Light, Heater, HangOnFilter)
  int pin;                        // GPIO pin for relay control
  ApplianceState currentState;    // Current physical state (ON/OFF)
  ApplianceState scheduledState;  // State according to schedule
  ApplianceState overrideState;   // State from manual override
  unsigned long overrideEndTime;  // millis() when override expires (0 = no override)
  ApplianceMode currentMode;      // Current control mode (SCHEDULED/OVERRIDDEN/TEMP_CONTROLLED)
};

// Array of 5 appliance objects with custom configurations
Appliance appliances[] = {
  { "Filter", MOTOR_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },          // Primary filtration (GPIO 18)
  { "CO2", CO2_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },               // CO2 injection system (GPIO 16)
  { "Light", LIGHT_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },           // LED lighting system (GPIO 5)
  { "Heater", HEATER_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },         // Water heater with temp control (GPIO 19)
  { "HangOnFilter", HANGON_FILTER_PIN, OFF, OFF, OFF, 0, SCHEDULED },  // Secondary hang-on filter (GPIO 17)
  { "WaveMaker", WAVE_MAKER_PIN, OFF, OFF, OFF, 0, SCHEDULED }         // Wave maker (GPIO 32)
};
const int NUM_APPLIANCES = sizeof(appliances) / sizeof(appliances[0]);

// Map to store custom schedules for each appliance (minute-based for efficiency)
std::map<String, std::vector<ScheduleEntry>> applianceSchedules;

// Global POST body storage (single reusable buffer for memory optimization)
String postBody = "";

// Move large constant strings to PROGMEM
const char PROGMEM STR_ERR_NVS_WIFI[] = "[NVS] Failed to initialize WiFi preferences.";
const char PROGMEM STR_ERR_RTC[] = "[ERROR] RTC not found!";
const char PROGMEM STR_ERR_OLED[] = "[ERROR] OLED init failed";
const char PROGMEM STR_RTC_NOT_RUNNING[] = "[RTC] RTC is NOT running, setting time from build time.";
const char PROGMEM STR_RTC_INIT[] = "[RTC] RTC initialized.";
const char PROGMEM STR_OLED_INIT[] = "[OLED] Display initialized.";
const char PROGMEM STR_API_STARTED[] = "[API] REST API server started.";
const char PROGMEM STR_SETUP_READY[] = "[SETUP] System ready!";
unsigned long lastDebugPrintMillis = 0;
const unsigned long DEBUG_PRINT_INTERVAL_MS = 30000;  // Print debug info every 30 seconds (reduced frequency)

// OLED Display System
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
unsigned long lastOLEDUpdateMillis = 0;
const unsigned long OLED_UPDATE_INTERVAL_MS = 2000;  // Update every 2 seconds (memory optimized)
char oledBuffer[64];                                 // Reusable buffer for OLED text formatting

// WiFi Connection Management
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 30000;  // 30 seconds for connection
bool wifiConnected = false;
bool apModeActive = false;
int wifiReconnectAttempts = 0;              // Track reconnection attempts
const int MAX_WIFI_RECONNECT_ATTEMPTS = 3;  // Reduced from 5 for memory optimization
unsigned long lastWifiReconnectMillis = 0;  // Separate timing for WiFi reconnection

// Emergency Safety System
bool emergencyShutdown = false;
const float EMERGENCY_TEMP_HIGH = 32.0;                        // Emergency shutdown if temperature exceeds 32°C
const float EMERGENCY_TEMP_LOW = 15.0;                         // Emergency shutdown if temperature below 15°C
bool lastEmergencyState = false;                               // Cache to avoid redundant operations
unsigned long emergencyActivatedTime = 0;                      // Track when emergency was activated
const unsigned long EMERGENCY_RESET_TIME_MS = 30 * 60 * 1000;  // Auto-reset emergency after 30 minutes

// Emergency Reason Tracking
enum EmergencyReason {
  NO_EMERGENCY = 0,
  TEMP_TOO_HIGH,
  TEMP_TOO_LOW,
  SENSOR_FAILURE
};
EmergencyReason currentEmergencyReason = NO_EMERGENCY;

// Schedule caching for performance
bool schedulesDirty = false;  // Flag to track if schedules need saving
unsigned long lastScheduleSaveMillis = 0;
const unsigned long SCHEDULE_SAVE_DELAY_MS = 5000;  // Save schedules 5 seconds after last change

// ============================================================================
// 4. Function Prototypes
// ============================================================================
// Core System Functions
void connectWiFi();
void startAPMode();
void buzz(int count, int delayMs);
void syncTimeNTP();

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
void handleUpdateSingleSchedule(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
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
  // Cache the current GPIO state to avoid redundant writes
  static int lastPinStates[5] = { -1, -1, -1, -1, -1 };   // Only track our 5 relay pins
  static const int relayPins[5] = { 18, 16, 5, 19, 17 };  // Our actual relay pins

  int newState = (state == ON) ? getRelayOnState() : getRelayOffState();

  // Find the pin index in our array
  for (int i = 0; i < 5; i++) {
    if (relayPins[i] == pin) {
      // Only write if state has changed
      if (lastPinStates[i] != newState) {
        digitalWrite(pin, newState);
        lastPinStates[i] = newState;
      }
      break;
    }
  }
}

// ============================================================================
// 6. Setup & Loop Functions
// ============================================================================
void setup() {
  // Add small delay for power stabilization
  delay(800);

  // Set CPU frequency for stability
  setCpuFrequencyMhz(160);

  // Disable brownout detector (only if power supply is insufficient)
  // WARNING: Only use this if you have a stable 3.3V supply
  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Uncomment only if needed

  Serial.begin(115200);
#if DEBUG_MODE
  Serial.println("\n[SETUP] Starting ESP32 Fish Tank Automation System...");
  Serial.printf("[SETUP] CPU Frequency: %dMHz\n", getCpuFrequencyMhz());
#endif

  // Add 1-second startup delay with beep indicator
  delay(1000);

  // Configure Buzzer Pin early for startup beep (ESP32 Core compatible)
  BUZZER_INIT(BUZZER_PIN, 1000, 8);  // Cross-compatible initialization

  // Startup beep to indicate system is ready
  BUZZER_TONE(BUZZER_PIN, 1500);  // 1.5kHz startup tone
  delay(300);                     // Beep duration
  BUZZER_OFF(BUZZER_PIN);         // Turn off
#if DEBUG_MODE
  Serial.println("[SETUP] System startup beep completed.");
#endif

  // Initialize NVS (WiFi namespace will be used initially)
  if (!preferences.begin(NVS_NAMESPACE_WIFI, false)) {
#if DEBUG_MODE
    Serial.println(FPSTR(STR_ERR_NVS_WIFI));
#endif
  }

  // Initialize I2C bus
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(100000);  // Standard mode (100kHz)
  Wire.setTimeout(5000);
  delay(100);  // Give I2C bus time to stabilize before accessing devices
#if DEBUG_MODE
  Serial.println(F("[I2C] I2C bus initialized."));
#endif

  int i = 0;
  // Initialize RTC
  if (!rtc.begin()) {
    rtcInitialized = false;
#if DEBUG_MODE
    Serial.println(FPSTR(STR_ERR_RTC));
#endif
    // Sound continuous error buzzer to alert user
    while (1) {
      i++;
      BUZZER_TONE(BUZZER_PIN, 3000);  // 3kHz error tone
      delay(800);                     // Buzz for 800ms
      BUZZER_OFF(BUZZER_PIN);         // Turn off
      delay(1000);                    // Wait 1 second before next buzz

      // Restart ESP if RTC not found
      if (i >= 10) {
        delay(1000);
        ESP.restart();
      }
    }
  }
  // Use rtc.lostPower() to check if RTC lost power and set time if needed
  if (rtc.lostPower()) {
#if DEBUG_MODE
    Serial.println(FPSTR(STR_RTC_NOT_RUNNING));
#endif
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set to compile time as fallback
  }
  rtcInitialized = true;  // Mark RTC as successfully initialized
#if DEBUG_MODE
  Serial.println(FPSTR(STR_RTC_INIT));
#endif

  // Initialize OLED Display
#if ENABLE_OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
    oledInitialized = false;
#if DEBUG_MODE
    Serial.println(FPSTR(STR_ERR_OLED));
#endif
  } else {
    oledInitialized = true;
#if DEBUG_MODE
    Serial.println(FPSTR(STR_OLED_INIT));
#endif
    delay(100);  // Let OLED power stabilize after initialization
    display.setRotation(1);  // Rotate display 90 degrees clockwise (64px wide × 128px tall)
    display.clearDisplay();
    display.setTextSize(1);  // Compact text for portrait mode
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 30);  // Center vertically
    display.println(F("Bismillah"));
    display.display();
    delay(2000);  // Show for 2 seconds
    display.clearDisplay();
    display.display();
  }
#else
  oledInitialized = false;
#if DEBUG_MODE
  Serial.println(F("[OLED] OLED disabled by compile flag"));
#endif
#endif

  // Initialize DS18B20 Temperature Sensor
  sensors.begin();
  sensors.setResolution(12);            // Set to 12-bit resolution for better accuracy
  sensors.setWaitForConversion(false);  // Use non-blocking mode

  // Initialize sensor state variables
  temperatureReadFailures = 0;
  temperatureRecoveryCount = 0;

  // Take an initial temperature reading
  sensors.requestTemperatures();
  delay(800);  // Wait 800ms for 12-bit conversion (750ms required + margin)
  float initialTemp = sensors.getTempCByIndex(0);
  if (initialTemp != DEVICE_DISCONNECTED_C && initialTemp > -50.0 && initialTemp < 100.0) {
    currentTemperatureC = initialTemp;
#if DEBUG_MODE
    Serial.printf("[DS18B20] Initial temperature reading: %.1f°C\n", currentTemperatureC);
#endif
  } else {
    currentTemperatureC = DEVICE_DISCONNECTED_C;
#if DEBUG_MODE
    Serial.println("[DS18B20] Warning: Initial temperature reading failed, using error value -127.0°C");
#endif
  }

#if DEBUG_MODE
  Serial.println("[DS18B20] Temperature sensor initialized and configured.");
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
  server.on("/restart", HTTP_GET, handleRestart);  // [GET] /restart endpoint for system reboot
  server.on(
    "/control", HTTP_POST, [](AsyncWebServerRequest *request) {
      // Response will be sent by the body handler
    },
    NULL, handleControl);
  server.on("/schedules", HTTP_GET, handleGetSchedules);
  server.on(
    "/schedules", HTTP_POST, [](AsyncWebServerRequest *request) {
      // Response will be sent by the body handler
    },
    NULL, handlePostSchedules);
  server.on(
    "/schedule", HTTP_POST, [](AsyncWebServerRequest *request) {
      // Response will be sent by the body handler for single appliance update
    },
    NULL, handleUpdateSingleSchedule);
  server.on(
    "/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
      // Response will be sent by the body handler
    },
    NULL, handleWifiConfig);
  server.on("/emergency/reset", HTTP_POST, handleEmergencyReset);
  server.on(
    "/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
      // Response will be sent by the body handler
    },
    NULL, handleResetToSchedule);
  server.onNotFound(handleNotFound);

  // Start the server
  server.begin();
#if DEBUG_MODE
  Serial.println(FPSTR(STR_API_STARTED));
#endif
#if DEBUG_MODE
  Serial.println(FPSTR(STR_SETUP_READY));
#endif
}

// =====================================
// SAFE JSON RESPONSE BUILDER
// =====================================
// Overload for regular char* strings
bool buildSafeJsonResponse(char *buffer, size_t bufferSize, const char *status, const char *message = nullptr) {
  size_t statusLen = strlen(status);
  size_t messageLen = message ? strlen(message) : 0;

  // Calculate minimum required size: {"status":"","message":""}
  size_t minSize = 21 + statusLen + messageLen;  // 21 chars for JSON structure

  if (minSize >= bufferSize) {
    // Buffer too small, use truncated message
    snprintf(buffer, bufferSize, "{\"status\":\"%.*s\"}",
             (int)(bufferSize - 15), status);  // Leave room for JSON structure
    return false;
  }

  if (message) {
    snprintf(buffer, bufferSize, "{\"status\":\"%s\",\"message\":\"%s\"}", status, message);
  } else {
    snprintf(buffer, bufferSize, "{\"status\":\"%s\"}", status);
  }
  return true;
}

// Overload for PROGMEM strings
bool buildSafeJsonResponse(char *buffer, size_t bufferSize, const __FlashStringHelper *status, const __FlashStringHelper *message = nullptr) {
  // Convert PROGMEM strings to regular strings for length calculation
  String statusStr = String(status);
  String messageStr = message ? String(message) : "";

  size_t statusLen = statusStr.length();
  size_t messageLen = messageStr.length();

  // Calculate minimum required size: {"status":"","message":""}
  size_t minSize = 21 + statusLen + messageLen;  // 21 chars for JSON structure

  if (minSize >= bufferSize) {
    // Buffer too small, use truncated message
    snprintf_P(buffer, bufferSize, PSTR("{\"status\":\"%.*s\"}"),
               (int)(bufferSize - 15), statusStr.c_str());  // Leave room for JSON structure
    return false;
  }

  if (message) {
    snprintf_P(buffer, bufferSize, PSTR("{\"status\":\"%s\",\"message\":\"%s\"}"),
               statusStr.c_str(), messageStr.c_str());
  } else {
    snprintf_P(buffer, bufferSize, PSTR("{\"status\":\"%s\"}"), statusStr.c_str());
  }
  return true;
}

/**
 * @brief Handles the "/restart" GET endpoint. Reboots the ESP32 after authenticating the API key.
 * Responds with JSON and then triggers ESP.restart().
 */
void handleRestart(AsyncWebServerRequest *request) {
  if (!authenticateRequest(request)) return;
  char jsonBuffer[96];
  if (buildSafeJsonResponse(jsonBuffer, sizeof(jsonBuffer), FPSTR(STR_SUCCESS), F("System will reboot now"))) {
    request->send(200, FPSTR(STR_CONTENT_TYPE_JSON), jsonBuffer);
  } else {
    request->send(200, FPSTR(STR_CONTENT_TYPE_JSON), "{\"status\":\"success\"}");
  }
  delay(1000);  // Allow response to be sent
  ESP.restart();
}

// ============================================================================
// 7. Loop Function
// ============================================================================
void loop() {
  // Handle WiFi reconnection if disconnected
  if (WiFi.status() != WL_CONNECTED && !apModeActive) {
    if ((long)(millis() - lastWifiReconnectMillis) > 10000) {  // Check every 10 seconds
      connectWiFi();
      lastWifiReconnectMillis = millis();
    }
  }

  // Periodically sync RTC with NTP (every 3 hours, if WiFi available)
  if (wifiConnected && ((long)(millis() - lastNtpSyncMillis) >= NTP_SYNC_INTERVAL_MS)) {
    syncTimeNTP();
    lastNtpSyncMillis = millis();
  }

  // Get current time from RTC
  if (!rtcInitialized) {
    // RTC not available, skip time-dependent operations
    return;
  }
  DateTime now = rtc.now();
  int currentMinutes = now.hour() * 60 + now.minute();  // Convert to minutes once

  // Read temperature more frequently if sensor error
  unsigned long tempReadInterval = tempSensorError ? TEMP_READ_ERROR_INTERVAL_MS : TEMP_READ_INTERVAL_MS;
  if ((long)(millis() - lastTempReadMillis) >= tempReadInterval) {
    sensors.requestTemperatures();
    delay(800);  // Wait 800ms for 12-bit conversion (750ms required + margin)
    float tempReading = sensors.getTempCByIndex(0);

    if (tempReading != DEVICE_DISCONNECTED_C && tempReading > -50.0 && tempReading < 100.0) {
      // Valid temperature reading
      currentTemperatureC = (currentTemperatureC * 0.9) + (tempReading * 0.1);
      if (tempSensorError) {
        tempSensorError = false;  // Sensor recovered
        lastTempSensorErrorBuzzMillis = 0;
      }
      // Track recovery after failures
      if (temperatureReadFailures > 0) {
        temperatureRecoveryCount++;
#if DEBUG_MODE
        Serial.printf(F("[TEMP] Recovery reading %d/%d (%.1f°C)\n"),
                      temperatureRecoveryCount, MIN_RECOVERY_READINGS, tempReading);
#endif
        // Only reset failures after multiple successful readings
        if (temperatureRecoveryCount >= MIN_RECOVERY_READINGS) {
          temperatureReadFailures = 0;
          temperatureRecoveryCount = 0;
#if DEBUG_MODE
          Serial.println(F("[TEMP] Sensor recovery confirmed - failures reset"));
#endif
        }
      } else if (temperatureReadFailures == 0) {
        // Only reset recovery counter when sensor is stable AND no failures exist
        temperatureRecoveryCount = 0;
      }
    } else {
      // Temperature sensor failure detected
      if (temperatureReadFailures < MAX_TEMP_FAILURES) {
        temperatureReadFailures++;
        temperatureRecoveryCount = 0;  // Reset recovery on new failure
        tempSensorError = true;
#if DEBUG_MODE
        Serial.printf(F("[TEMP] Sensor read failure %d/%d (Reading: %.1f)\n"),
                      temperatureReadFailures, MAX_TEMP_FAILURES, tempReading);
#endif
        // No emergency shutdown for temp sensor failure; just log and warn (handled elsewhere)
      }
    }
    lastTempReadMillis = millis();
  }

  // DS18B20 Sensor Error: Spontaneous 3-buzz warning every hour
  if (tempSensorError) {
    if (lastTempSensorErrorBuzzMillis == 0 || (long)(millis() - lastTempSensorErrorBuzzMillis) >= TEMP_SENSOR_ERROR_BUZZ_INTERVAL_MS) {
      buzz(3, 300);  // 3 quick buzzes
      lastTempSensorErrorBuzzMillis = millis();
    }
  }

  // Check emergency conditions - exclude zero/invalid values to prevent false alarms
  bool currentEmergencyState = false;
  EmergencyReason newEmergencyReason = NO_EMERGENCY;

  if (currentTemperatureC != -127.0) {
    // Only check temperature limits if we have valid readings
    if (currentTemperatureC > EMERGENCY_TEMP_HIGH) {
      currentEmergencyState = true;
      newEmergencyReason = TEMP_TOO_HIGH;
    } else if (currentTemperatureC < EMERGENCY_TEMP_LOW) {
      currentEmergencyState = true;
      newEmergencyReason = TEMP_TOO_LOW;
    }
  }
  // Do NOT trigger emergency for temp sensor failure anymore

  // Update emergency reason when state changes
  if (currentEmergencyState) {
    currentEmergencyReason = newEmergencyReason;
  }

  // Auto-reset emergency after timeout or when conditions are safe
  if (emergencyShutdown && emergencyActivatedTime > 0) {
    bool timeoutReset = (long)(millis() - emergencyActivatedTime) >= EMERGENCY_RESET_TIME_MS;
    bool conditionsSafe = (currentTemperatureC > 0.0 && currentTemperatureC > EMERGENCY_TEMP_LOW && currentTemperatureC < EMERGENCY_TEMP_HIGH && temperatureReadFailures == 0 && temperatureRecoveryCount == 0);  // Only reset when sensor is fully stable

    if (timeoutReset || conditionsSafe) {
      emergencyShutdown = false;
      emergencyActivatedTime = 0;
      temperatureReadFailures = 0;            // Reset failure counter
      temperatureRecoveryCount = 0;           // Reset recovery counter
      currentEmergencyReason = NO_EMERGENCY;  // Reset emergency reason
#if DEBUG_MODE
      Serial.println(timeoutReset ? F("[EMERGENCY] Auto-reset after timeout - System resumed") : F("[EMERGENCY] Auto-reset: Conditions safe - System resumed"));
#endif
      buzz(2, 300);  // Confirmation buzzes
    }
  }

  if (currentEmergencyState != lastEmergencyState) {
    emergencyShutdown = currentEmergencyState;
    lastEmergencyState = currentEmergencyState;
    if (emergencyShutdown) {
      emergencyActivatedTime = millis();
#if DEBUG_MODE
      Serial.printf(F("[EMERGENCY] Temperature %.1f°C or sensor failure - Shutting down all appliances\n"),
                    currentTemperatureC);
#endif
    }
  }

  // Apply appliance control logic (unless emergency shutdown)
  if (!emergencyShutdown) {
    for (int i = 0; i < NUM_APPLIANCES; i++) {
      applyApplianceLogic(appliances[i], currentMinutes);
    }
  }

// Conditional debug output (only if DEBUG_MODE enabled and reduced frequency)
#if DEBUG_MODE
  if ((long)(millis() - lastDebugPrintMillis) >= DEBUG_PRINT_INTERVAL_MS) {
    Serial.printf(F("[DEBUG] %02d:%02d T:%.1f°C Heap:%u Emergency:%s\n"),
                  now.hour(), now.minute(), currentTemperatureC, ESP.getFreeHeap(),
                  emergencyShutdown ? "Y" : "N");
    lastDebugPrintMillis = millis();
  }
#endif

  // Update OLED display periodically (for time updates when no state changes)
#if ENABLE_OLED
  if ((long)(millis() - lastOLEDUpdateMillis) >= OLED_UPDATE_INTERVAL_MS) {
    updateOLED(now);
    lastOLEDUpdateMillis = millis();
  }
#endif

  // Handle delayed schedule saving
  handleDelayedScheduleSave();
}

// ============================================================================
// 8. Core Logic Functions (WiFi, Scheduling, Relay Control)
// ============================================================================

/**
 * @brief Connects to a saved WiFi network
 */
void connectWiFi() {
  // Check if we've exceeded reconnection attempts
  if (wifiReconnectAttempts >= MAX_WIFI_RECONNECT_ATTEMPTS) {
    wifiConnected = false;
    return;
  }

  // Switch to WiFi namespace to read credentials
  preferences.end();  // End current namespace
  if (!preferences.begin(NVS_NAMESPACE_WIFI, false)) {
    Serial.println(F("[NVS] Failed to open WiFi namespace."));
    wifiConnected = false;
    wifiReconnectAttempts++;
    return;
  }

  String ssid = preferences.getString(NVS_KEY_SSID, "");
  String password = preferences.getString(NVS_KEY_PASS, "");

  if (ssid.length() == 0) {
#if DEBUG_MODE
    Serial.println(F("[WiFi] No saved WiFi credentials. Cannot connect to STA mode."));
#endif
    wifiConnected = false;
    return;
  }

#if DEBUG_MODE
  Serial.printf(F("[WiFi] Attempting to connect to SSID: %s (Attempt %d/%d)\n"),
                ssid.c_str(), wifiReconnectAttempts + 1, MAX_WIFI_RECONNECT_ATTEMPTS);
#endif
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  // Reduce WiFi power to prevent brownouts during transmission
  // WiFi.setTxPower(WIFI_POWER_15dBm);  // Reduce from default 20dBm to 15dBm
  delay(1000);
  WiFi.begin(ssid.c_str(), password.c_str());
  delay(2000);

  unsigned long startTime = millis();
  int dotCount = 0;
  // For scrolling SSID
  static int ssidScrollOffset = 0;
  static unsigned long lastScrollMillis = 0;
  const int scrollDelay = 250;  // ms per scroll step
  while (WiFi.status() != WL_CONNECTED && (long)(millis() - startTime) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(1000);
#if DEBUG_MODE
    Serial.print(".");
#endif
    // Update display every 3 seconds
#if ENABLE_OLED
    if (++dotCount % 3 == 0) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      
      // Portrait mode: 64px wide (10 chars max)
      int y = 0;
      display.setCursor(0, y);
      display.println(F("Connect:"));
      y += 10;
      
      // Scroll SSID if too long
      int maxVisibleChars = 10;  // 64px/6px = 10 chars
      int ssidLen = ssid.length();
      String ssidToShow;
      if (ssidLen > maxVisibleChars) {
        // Scroll the SSID horizontally
        if ((long)(millis() - lastScrollMillis) > scrollDelay) {
          if (ssidLen > 0) {
            ssidScrollOffset = (ssidScrollOffset + 1) % (ssidLen + 2);
          } else {
            ssidScrollOffset = 0;
          }
          lastScrollMillis = millis();
        }
        // Bounds checking for substring operations
        if (ssidScrollOffset >= 0 && ssidScrollOffset < ssidLen && ssidScrollOffset + maxVisibleChars <= ssidLen) {
          ssidToShow = ssid.substring(ssidScrollOffset, ssidScrollOffset + maxVisibleChars);
        } else if (ssidScrollOffset >= 0 && ssidScrollOffset < ssidLen) {
          int firstPart = ssidLen - ssidScrollOffset;
          if (firstPart > 0 && firstPart <= ssidLen) {
            ssidToShow = ssid.substring(ssidScrollOffset);
            ssidToShow += "  ";
            int remainingChars = maxVisibleChars - firstPart - 2;
            if (remainingChars > 0 && remainingChars <= ssidLen) {
              ssidToShow += ssid.substring(0, remainingChars);
            }
          } else {
            ssidToShow = ssid.substring(0, min(maxVisibleChars, ssidLen));
          }
        } else {
          ssidScrollOffset = 0;
          ssidToShow = ssid.substring(0, min(maxVisibleChars, ssidLen));
        }
      } else {
        ssidToShow = ssid;
      }
      display.setCursor(0, y);
      display.println(ssidToShow);
      y += 10;
      
      snprintf(oledBuffer, sizeof(oledBuffer), "Try %d/%d", wifiReconnectAttempts + 1, MAX_WIFI_RECONNECT_ATTEMPTS);
      display.setCursor(0, y);
      display.println(oledBuffer);
      display.display();
    }
#endif
  }

  if (WiFi.status() == WL_CONNECTED) {
#if DEBUG_MODE
    Serial.println(F("\n[WiFi] Connected to WiFi!"));
    Serial.print(F("[WiFi] IP Address: "));
    Serial.println(WiFi.localIP());
#endif
    wifiConnected = true;
    apModeActive = false;
    wifiReconnectAttempts = 0;  // Reset attempt counter on successful connection
  } else {
#if DEBUG_MODE
    Serial.println(F("\n[WiFi] Failed to connect to WiFi."));
#endif
    wifiConnected = false;
    wifiReconnectAttempts++;
  }

  // Close WiFi namespace
  preferences.end();
}

/**
 * @brief Starts the ESP32 in Access Point (AP) mode for configuration.
 */
void startAPMode() {
#if DEBUG_MODE
  Serial.println("[AP] Starting Access Point mode...");
#endif
  WiFi.mode(WIFI_AP);
  // Set constant AP IP: 192.168.0.1 (gateway and subnet match)
  IPAddress apIP(192, 168, 0, 1);
  IPAddress apGateway(192, 168, 0, 1);
  IPAddress apSubnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apGateway, apSubnet);
  WiFi.softAP("MyTank", "password@123");  // Default AP name and password
  apIP = WiFi.softAPIP();
#if DEBUG_MODE
  Serial.print("[AP] AP IP address: ");
  Serial.println(apIP);
#endif
  apModeActive = true;

#if ENABLE_OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Portrait mode: 64px wide (10 chars max)
  int y = 0;
  display.setCursor(0, y);
  display.println(F("AP Mode"));
  y += 10;
  
  display.setCursor(0, y);
  display.println(F("MyTank"));
  y += 10;
  
  display.setCursor(0, y);
  display.println(F("pass@123"));
  y += 10;
  
  display.setCursor(0, y);
  snprintf(oledBuffer, sizeof(oledBuffer), "%d.%d.%d.%d", apIP[0], apIP[1], apIP[2], apIP[3]);
  display.println(oledBuffer);
  
  display.display();
#endif
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
    BUZZER_TONE(BUZZER_PIN, 1000);  // 1kHz tone
    delay(200);                     // Buzz duration
    BUZZER_OFF(BUZZER_PIN);         // Turn off
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
 * @brief Loads the default factory schedules and saves them to NVS.
 * This function is used for initial setup and factory reset.
 */
void loadDefaultSchedules() {
  // Clear existing schedules
  applianceSchedules.clear();

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

  // Hang-on Filter: 9:00 AM - 10:30 AM (540-630), 7:30 PM - 8:30 PM (930-1230)
  applianceSchedules["HangOnFilter"].push_back({ "on_interval", 540, 630 });
  applianceSchedules["HangOnFilter"].push_back({ "on_interval", 930, 1230 });

  // WaveMaker Filter: 10:30 AM - 12:30 PM (630-750), 3:30 PM - 7:30 PM (930-1170)
  applianceSchedules["WaveMaker"].push_back({ "on_interval", 630, 750 });
  applianceSchedules["WaveMaker"].push_back({ "on_interval", 930, 1170 });

  // Filter: 1:30 PM - 3:30 PM (810-930) - OFF during this time for maintenance
  applianceSchedules["Filter"].push_back({ "off_interval", 810, 930 });

  markSchedulesDirty();  // Schedule delayed save

#if DEBUG_MODE
  Serial.println(F("[Factory Reset] Default schedules loaded:"));
  Serial.println(F("  CO2: 8:30-13:30 (510-810), 15:30-20:30 (930-1230)"));
  Serial.println(F("  Light: 9:30-13:30 (570-810), 16:30-20:30 (990-1230)"));
  Serial.println("  Heater: 0:00-4:30 (0-270), 20:30-23:59 (1230-1439)");
  Serial.println("  HangOnFilter: 9:00-10:30 (540-630), 19:30-20:30 (930-1230)");
  Serial.println("  WaveMaker: 10:30-12:30 (630-750), 15:30-19:30 (930-1170)");
  Serial.println("  Filter: OFF 13:30-15:30 (810-930), otherwise ON");
#endif
}

/**
 * @brief Loads appliance schedules from NVS. If no schedules are found,
 * custom default schedules optimized for fish tank automation are loaded and saved to NVS.
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
    loadDefaultSchedules();
    return;
  }

#if DEBUG_MODE
  Serial.println("[NVS] Loading schedules from NVS...");
#endif
  StaticJsonDocument<2048> doc;  // Use StaticJsonDocument for ArduinoJson v6
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
      if (scheduleObj["start_min"].is<int>()) {
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

  // Close schedules namespace
  preferences.end();
}

/**
 * @brief Saves current appliance schedules to NVS
 */
void saveSchedules() {
  // Switch to schedules namespace
  preferences.end();  // End current namespace
  if (!preferences.begin(NVS_NAMESPACE_SCHEDULES, false)) {
    Serial.println(F("[NVS] Failed to open schedules namespace for saving."));
    return;
  }

  StaticJsonDocument<2048> doc;  // Use StaticJsonDocument with adequate size

  for (auto const &[applianceName, schedulesVec] : applianceSchedules) {
    JsonArray schedulesArray = doc[applianceName].to<JsonArray>();
    for (const auto &entry : schedulesVec) {
      JsonObject scheduleObj = schedulesArray.add<JsonObject>();
      scheduleObj["type"] = entry.type;
      scheduleObj["start_min"] = entry.start_min;
      scheduleObj["end_min"] = entry.end_min;
    }
  }

  String schedulesJson;
  schedulesJson.reserve(RESPONSE_BUFFER_SIZE);  // Pre-allocate memory
  serializeJson(doc, schedulesJson);
  preferences.putString(NVS_KEY_SCHEDULES, schedulesJson);

#if DEBUG_MODE
  Serial.println(F("[NVS] Schedules saved successfully."));
#endif

  schedulesDirty = false;  // Reset dirty flag
  preferences.end();
}

/**
 * @brief Marks schedules as dirty and schedules a delayed save
 */
void markSchedulesDirty() {
  schedulesDirty = true;
  lastScheduleSaveMillis = millis();
}

/**
 * @brief Handles delayed schedule saving
 */
void handleDelayedScheduleSave() {
  if (schedulesDirty && (long)(millis() - lastScheduleSaveMillis) >= SCHEDULE_SAVE_DELAY_MS) {
    saveSchedules();
  }
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
            isScheduledOff = false;
          } else if (schedule.type == "off_interval") {
            isScheduledOff = true;
            isScheduledOn = false;
          }
        }
      }

      // // Apply schedule logic based on appliance type
      // if (app.name == "Filter") {
      //   // Filter: ON by default, turn OFF only during off_interval
      //   targetState = isScheduledOff ? OFF : ON;
      // } else {
      //   // Other appliances: OFF by default, turn ON only during on_interval
      //   targetState = isScheduledOn ? ON : OFF;
      // }
    }
    app.scheduledState = targetState;
  }

  // 3. Intelligent Heater Temperature Logic (manual override has highest priority)
  if (app.name == "Heater") {
    // IMPORTANT: Skip temperature control entirely if manual override is active
    if (app.overrideEndTime > 0) {
      // Manual override is active - use override state, skip all temperature logic
      // targetState already set from override section above
      // Temperature control variables will reset naturally when override expires
    } else {
      // No override - apply temperature control logic
      // Critical section for heater control - prevent race conditions
      if (heaterControlLock) {
        // Skip this update if already in progress (will be processed next cycle)
        return;
      }
      heaterControlLock = true;

      if (tempSensorError) {
        // Sensor error: fall back to scheduled state
        targetState = app.scheduledState;
        app.currentMode = SCHEDULED;
        heaterForcedOn = false;
        heaterOnTimeMillis = 0;
      } else if (currentTemperatureC > 0) {
        bool currentHeaterState = (targetState == ON);
        if (currentTemperatureC < TEMP_THRESHOLD_ON) {
          // Temperature too low - force heater ON
          if (!currentHeaterState && !heaterForcedOn) {
            heaterOnTimeMillis = millis();
            heaterForcedOn = true;
          }
          targetState = ON;
          app.currentMode = TEMP_CONTROLLED;
        } else if (currentTemperatureC >= TEMP_THRESHOLD_OFF) {
          // Temperature reached target - check minimum runtime
          if (heaterForcedOn && ((long)(millis() - heaterOnTimeMillis) < HEATER_MIN_RUN_TIME_MS)) {
            targetState = ON;  // Keep ON for minimum 30-minute runtime
            app.currentMode = TEMP_CONTROLLED;
          } else {
            heaterForcedOn = false;
            heaterOnTimeMillis = 0;
            targetState = OFF;
            app.currentMode = (app.overrideEndTime > 0) ? OVERRIDDEN : SCHEDULED;
          }
        }
        // Ensure minimum runtime is respected (optimized check)
        if (heaterForcedOn && targetState == OFF) {
          targetState = ON;
          app.currentMode = TEMP_CONTROLLED;
        }
      }

      // ALWAYS release lock at end of Heater control section
      heaterControlLock = false;
    }
  }

  // Update appliance state only if changed (optimized)
  if (app.currentState != targetState) {
    app.currentState = targetState;
    setRelayState(app.pin, app.currentState);  // Use configurable relay control
                                               // Immediately update OLED to reflect the change
#if ENABLE_OLED
    DateTime now = rtc.now();
    updateOLED(now);
#endif
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
/**
 * @brief Updates OLED display with current status
 */
void updateOLED(DateTime now) {
  // Skip OLED updates if display not initialized
  if (!oledInitialized) {
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // After rotation(1): 64px wide × 128px tall (portrait mode)
  // Font size 1: 6px wide × 8px tall per char
  // Max chars per line: 10 chars (64px/6px = 10.6)
  int y = 0;

  // Row 1: Time (HH:MM:SS)
  display.setCursor(0, y);
  snprintf(oledBuffer, sizeof(oledBuffer), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  display.print(oledBuffer);
  y += 10;

  // Row 2: Date (DD/MM)
  display.setCursor(0, y);
  snprintf(oledBuffer, sizeof(oledBuffer), "%02d/%02d", now.day(), now.month());
  display.print(oledBuffer);
  y += 10;

  // Appliances (compact: 2-3 char names)
  for (int i = 0; i < NUM_APPLIANCES; i++) {
    display.setCursor(0, y);
    const char *shortName;
    if (appliances[i].name == "Filter") shortName = "Fil";
    else if (appliances[i].name == "HangOnFilter") shortName = "HOF";
    else if (appliances[i].name == "CO2") shortName = "CO2";
    else if (appliances[i].name == "Light") shortName = "Lit";
    else if (appliances[i].name == "WaveMaker") shortName = "Wav";
    else if (appliances[i].name == "Heater") shortName = "Het";
    else shortName = appliances[i].name.c_str();

    snprintf(oledBuffer, sizeof(oledBuffer), "%s:%s", shortName,
             (appliances[i].currentState == ON ? "ON" : "OF"));
    display.print(oledBuffer);
    y += 9;
  }

  // Temperature & Status
  display.setCursor(0, y);
  if (tempSensorError) {
    // Marquee for sensor error
    static unsigned long lastScrollMillis = 0;
    static int scrollOffset = 0;
    const char *errorMsg = "SENSOR ERR!  ";
    int maxVisibleChars = 10;
    int errorMsgLen = strlen(errorMsg);
    
    if ((long)(millis() - lastScrollMillis) > 250) {
      scrollOffset = (scrollOffset + 1) % errorMsgLen;
      lastScrollMillis = millis();
    }
    char scrollBuf[12];
    for (int i = 0; i < maxVisibleChars; i++) {
      scrollBuf[i] = errorMsg[(scrollOffset + i) % errorMsgLen];
    }
    scrollBuf[maxVisibleChars] = '\0';
    display.print(scrollBuf);
  } else {
    snprintf(oledBuffer, sizeof(oledBuffer), "%.1fC", currentTemperatureC);
    display.print(oledBuffer);
  }
  y += 10;

  // System Status
  display.setCursor(0, y);
  if (emergencyShutdown) {
    switch (currentEmergencyReason) {
      case TEMP_TOO_HIGH: display.print(F("T-HIGH")); break;
      case TEMP_TOO_LOW: display.print(F("T-LOW")); break;
      case SENSOR_FAILURE: display.print(F("S-FAIL")); break;
      default: display.print(F("EMRG!")); break;
    }
  } else if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    snprintf(oledBuffer, sizeof(oledBuffer), "%d.%d", ip[2], ip[3]);
    display.print(oledBuffer);
  } else if (apModeActive) {
    display.print(F("AP"));
  } else {
    display.print(F("NoWiFi"));
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

// ============================================================================
// 9. REST API Handlers (All Grouped Together)
// ============================================================================

/**
 * @brief Authenticates incoming API requests using the API_KEY.
 * @param request Pointer to the AsyncWebServerRequest object.
 * @return True if authenticated, false otherwise.
 */
bool authenticateRequest(AsyncWebServerRequest *request) {
  if (request->hasHeader("X-API-Key") && request->header("X-API-Key") == API_KEY) {
    return true;
  }
  String json;
  json.reserve(96);
  json = "{\"status\": \"";
  json += FPSTR(STR_ERROR);
  json += "\", \"message\": \"Missing or invalid X-API-Key header.\"}";
  request->send(401, FPSTR(STR_CONTENT_TYPE_JSON), json);
  return false;
}

/**
 * @brief Handles the root ("/") endpoint.
 */
void handleRoot(AsyncWebServerRequest *request) {
  String msg;
  msg.reserve(96);
  msg = "ESP32 Fish Tank Automation System API. Use /status, /control, /schedules, /wifi.";
  request->send(200, "text/plain", msg);
}

/**
 * @brief Handles the "/status" GET endpoint. Returns current system status.
 */
void handleStatus(AsyncWebServerRequest *request) {
  if (!authenticateRequest(request)) return;

  StaticJsonDocument<1024> doc;  // Use StaticJsonDocument for ArduinoJson v6
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

  // Add emergency and sensor status fields
  doc["emergency"] = emergencyShutdown;
  // Add emergency reason as string
  const char *reasonStr = "NONE";
  switch (currentEmergencyReason) {
    case TEMP_TOO_HIGH: reasonStr = "TEMP_TOO_HIGH"; break;
    case TEMP_TOO_LOW: reasonStr = "TEMP_TOO_LOW"; break;
    case SENSOR_FAILURE: reasonStr = "SENSOR_FAILURE"; break;
    default: reasonStr = "NONE"; break;
  }
  doc["emergency_reason"] = reasonStr;
  doc["sensor_error"] = tempSensorError;

  // Add hardware status
  doc["rtc_initialized"] = rtcInitialized;
  doc["oled_initialized"] = oledInitialized;

  // Update OLED to ensure it matches the API response
#if ENABLE_OLED
  updateOLED(now);
#endif

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
    String json;
    json.reserve(64);
    json = "{\"status\": \"";
    json += FPSTR(STR_ERROR);
    json += "\", \"message\": \"Request body too large\"}";
    request->send(413, FPSTR(STR_CONTENT_TYPE_JSON), json);
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
    StaticJsonDocument<1024> doc;  // Use StaticJsonDocument for ArduinoJson v6
    DeserializationError error = deserializeJson(doc, postBody);

    if (error) {
      Serial.print(F("[API] JSON parse failed: "));
      Serial.println(error.f_str());
      String json;
      json.reserve(64);
      json = "{\"status\": \"";
      json += FPSTR(STR_ERROR);
      json += "\", \"message\": \"Invalid JSON\"}";
      request->send(400, FPSTR(STR_CONTENT_TYPE_JSON), json);
      return;
    }

    JsonArray appliancesArray = doc["appliances"].as<JsonArray>();

    if (appliancesArray.isNull()) {
      String json;
      json.reserve(64);
      json = "{\"status\": \"";
      json += FPSTR(STR_ERROR);
      json += "\", \"message\": \"Missing 'appliances' array in JSON body\"}";
      request->send(400, FPSTR(STR_CONTENT_TYPE_JSON), json);
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
          // Additional overflow protection: check if calculation would overflow
          if (timeoutMinutes > 0) {
            unsigned long timeoutMs = (unsigned long)timeoutMinutes * 60UL * 1000UL;
            unsigned long currentMillis = millis();
            // Check for potential overflow before addition
            if (timeoutMs > (ULONG_MAX - currentMillis)) {
              // Would overflow, set to maximum safe value
              appliances[i].overrideEndTime = ULONG_MAX;
            } else {
              appliances[i].overrideEndTime = currentMillis + timeoutMs;
            }
          } else {
            appliances[i].overrideEndTime = PERMANENT_OVERRIDE_VALUE;
          }
          break;
        }
      }
      if (!found) {
        char jsonBuffer[128];
        if (buildSafeJsonResponse(jsonBuffer, sizeof(jsonBuffer), FPSTR(STR_ERROR), FPSTR(STR_APPLIANCE_NOT_FOUND))) {
          request->send(404, FPSTR(STR_CONTENT_TYPE_JSON), jsonBuffer);
        } else {
          request->send(404, FPSTR(STR_CONTENT_TYPE_JSON), "{\"error\":\"Buffer overflow\"}");
        }
        return;
      }
    }
    String json;
    json.reserve(64);
    json = "{\"status\": \"";
    json += FPSTR(STR_SUCCESS);
    json += "\", \"message\": \"Appliance control updated\"}";
    request->send(200, FPSTR(STR_CONTENT_TYPE_JSON), json);
  }
}

/**
 * @brief Handles the "/schedules" GET endpoint. Returns all stored schedules. (Optimized)
 */
void handleGetSchedules(AsyncWebServerRequest *request) {
  if (!authenticateRequest(request)) return;

  // Use StaticJsonDocument for better memory efficiency
  StaticJsonDocument<2048> doc;  // Use StaticJsonDocument for ArduinoJson v6
  JsonObject schedules_json = doc["schedules"].to<JsonObject>();

  for (auto const &[applianceName, schedulesVec] : applianceSchedules) {
    JsonArray schedulesArray = schedules_json[applianceName].to<JsonArray>();
    for (const auto &entry : schedulesVec) {
      JsonObject scheduleObj = schedulesArray.add<JsonObject>();
      scheduleObj["type"] = entry.type;
      scheduleObj["start_min"] = entry.start_min;
      scheduleObj["end_min"] = entry.end_min;
    }
  }

  String response;
  response.reserve(RESPONSE_BUFFER_SIZE);  // Pre-allocate to avoid reallocation
  serializeJson(doc, response);
  request->send(200, FPSTR(STR_CONTENT_TYPE_JSON), response);
}

/**
 * @brief Handles the "/schedules" POST endpoint. Allows updating all schedules.
 * JSON Input:
 * {
 * "schedules": {
 * "Filter": [
 * {"type": "off_interval", "start_min": 570, "end_min": 630}
 * ],
 * "CO2": [
 * {"type": "on_interval", "start_min": 420, "end_min": 840},
 * {"type": "on_interval", "start_min": 900, "end_min": 1260}
 * ]
 * },
 * "clear_overrides": true  // Optional: clear all active overrides
 * }
 */
void handlePostSchedules(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (!authenticateRequest(request)) return;

  // Check total size limit
  if (total > MAX_POST_BODY_SIZE) {
    String json;
    json.reserve(64);
    json = "{\"status\": \"";
    json += FPSTR(STR_ERROR);
    json += "\", \"message\": \"Request body too large\"}";
    request->send(413, FPSTR(STR_CONTENT_TYPE_JSON), json);
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
    StaticJsonDocument<2048> doc;  // Use StaticJsonDocument for ArduinoJson v6
    DeserializationError error = deserializeJson(doc, postBody);

    if (error) {
      Serial.print(F("[API] deserializeJson() failed for /schedules: "));
      Serial.println(error.f_str());
      Serial.print(F("[API] Received body: "));
      Serial.println(postBody);
      String json;
      json.reserve(64);
      json = "{\"status\": \"";
      json += FPSTR(STR_ERROR);
      json += "\", \"message\": \"Invalid JSON\"}";
      request->send(400, FPSTR(STR_CONTENT_TYPE_JSON), json);
      return;
    }

    JsonObject incomingSchedules = doc["schedules"].as<JsonObject>();
    if (incomingSchedules.isNull()) {
      String json;
      json.reserve(64);
      json = "{\"status\": \"";
      json += FPSTR(STR_ERROR);
      json += "\", \"message\": \"Missing 'schedules' object in JSON body\"}";
      request->send(400, FPSTR(STR_CONTENT_TYPE_JSON), json);
      return;
    }

    // Optional: Clear all overrides when updating schedules
    bool clearOverrides = doc["clear_overrides"] | false;
    if (clearOverrides) {
      for (int i = 0; i < NUM_APPLIANCES; i++) {
        appliances[i].overrideEndTime = 0;  // Clear all overrides
        appliances[i].overrideState = OFF;
      }
      Serial.println("[API] All appliance overrides cleared due to clear_overrides flag");
    }

    applianceSchedules.clear();  // Clear all existing schedules

    for (JsonPair p : incomingSchedules) {
      String applianceName = p.key().c_str();
      JsonArray schedulesArray = p.value().as<JsonArray>();

      for (JsonObject scheduleObj : schedulesArray) {
        ScheduleEntry entry;
        entry.type = scheduleObj["type"].as<String>();
        // Convert hours and minutes to total minutes, or use direct minutes if provided
        if (scheduleObj["start_min"].is<int>()) {
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

    markSchedulesDirty();  // Schedule delayed save

    // Immediately re-evaluate all appliances with new schedules
    DateTime now = rtc.now();
    int currentMinutes = now.hour() * 60 + now.minute();
    for (int i = 0; i < NUM_APPLIANCES; i++) {
      applyApplianceLogic(appliances[i], currentMinutes);
    }

    String json;
    json.reserve(96);
    json = "{\"status\": \"";
    json += FPSTR(STR_SUCCESS);
    json += "\", \"message\": \"Schedules updated and applied immediately\"}";
    request->send(200, FPSTR(STR_CONTENT_TYPE_JSON), json);
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
    String json;
    json.reserve(64);
    json = "{\"status\": \"";
    json += FPSTR(STR_ERROR);
    json += "\", \"message\": \"Request body too large\"}";
    request->send(413, FPSTR(STR_CONTENT_TYPE_JSON), json);
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
    StaticJsonDocument<512> doc;  // WiFi config - small size needed
    DeserializationError error = deserializeJson(doc, postBody);

    if (error) {
      Serial.print(F("[API] deserializeJson() failed for /wifi: "));
      Serial.println(error.f_str());
      Serial.print(F("[API] Received body: "));
      Serial.println(postBody);
      String json;
      json.reserve(64);
      json = "{\"status\": \"";
      json += FPSTR(STR_ERROR);
      json += "\", \"message\": \"Invalid JSON\"}";
      request->send(400, FPSTR(STR_CONTENT_TYPE_JSON), json);
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
        String json;
        json.reserve(96);
        json = "{\"status\": \"";
        json += FPSTR(STR_SUCCESS);
        json += "\", \"message\": \"WiFi credentials saved. Rebooting to connect...\"}";
        request->send(200, FPSTR(STR_CONTENT_TYPE_JSON), json);
        delay(1000);    // Give time for response to send
        ESP.restart();  // Soft reboot to connect to new WiFi
      } else {
        String json;
        json.reserve(64);
        json = "{\"status\": \"";
        json += FPSTR(STR_ERROR);
        json += "\", \"message\": \"Failed to save WiFi credentials\"}";
        request->send(500, FPSTR(STR_CONTENT_TYPE_JSON), json);
      }
    } else {
      String json;
      json.reserve(64);
      json = "{\"status\": \"";
      json += FPSTR(STR_ERROR);
      json += "\", \"message\": \"SSID cannot be empty\"}";
      request->send(400, FPSTR(STR_CONTENT_TYPE_JSON), json);
    }
  }
}

/**
 * @brief Handles requests for unknown endpoints.
 */
void handleNotFound(AsyncWebServerRequest *request) {
  String msg;
  msg.reserve(32);
  msg = "Not Found";
  request->send(404, "text/plain", msg);
}

// ============================================================================
// 10. Emergency & System Health Functions
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
    BUZZER_TONE(BUZZER_PIN, 2000);
    delay(200);
    BUZZER_OFF(BUZZER_PIN);
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
 * @brief Handles the "/reset" POST endpoint. 
 * - Resets specific appliance overrides back to scheduled behavior, OR
 * - Performs factory reset: clears all overrides and restores default schedules
 * 
 * JSON Input for specific appliances:
 * {
 * "appliances": ["Light", "CO2"]  // Optional: specific appliances to reset
 * }
 * 
 * JSON Input for factory reset:
 * {
 * "reset_all": true  // Clears all overrides and restores default schedules
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
    StaticJsonDocument<512> doc;  // Use StaticJsonDocument for ArduinoJson v6
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
      // Factory Reset: Reset all appliances and restore default schedules
      for (int i = 0; i < NUM_APPLIANCES; i++) {
        appliances[i].overrideEndTime = 0;  // Clear override
        appliances[i].overrideState = OFF;  // Reset override state
        resetCount++;
      }

      // Load default factory schedules
      loadDefaultSchedules();

#if DEBUG_MODE
      Serial.println("[API] Factory reset completed: All appliance overrides cleared and default schedules restored");
#endif
    } else if (!appliancesToReset.isNull()) {
      // Reset specific appliances
      for (JsonVariant applianceName : appliancesToReset) {
        String appName = applianceName.as<String>();

        bool found = false;
        for (int i = 0; i < NUM_APPLIANCES; i++) {
          if (appliances[i].name == appName) {
            appliances[i].overrideEndTime = 0;  // Clear override
            appliances[i].overrideState = OFF;  // Reset override state
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

    String responseMessage;
    if (resetAll) {
      responseMessage = "Factory reset completed: " + String(resetCount) + " appliances reset and default schedules restored";
    } else {
      responseMessage = String(resetCount) + " appliances reset to schedule";
    }

    request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"" + responseMessage + " (System will reboot)\"}\n");
    delay(1000);  // Allow response to be sent
    ESP.restart();
  }
}

/**
 * @brief Handles the "/schedule" POST endpoint. Updates schedule for a single appliance.
 * JSON Input:
 * {
 * "appliance": "Light",
 * "schedule": [
 * {"type": "on_interval", "start_min": 420, "end_min": 1080}
 * ],
 * "clear_override": true  // Optional: clear any active override for this appliance
 * }
 */
void handleUpdateSingleSchedule(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (!authenticateRequest(request)) return;

  // Check total size limit
  if (total > MAX_POST_BODY_SIZE) {
    request->send(413, "application/json", "{\"error\": \"Request body too large\"}\n");
    return;
  }

  // Store the POST body data
  if (index == 0) {
    postBody = "";
    if (total > 0) {
      postBody.reserve(total + 1);  // Reserve memory only if total > 0
    }
  }
  for (size_t i = 0; i < len; i++) {
    postBody += (char)data[i];
  }

  // Only process when we have received all the data
  if (index + len == total) {
    StaticJsonDocument<1024> doc;  // Use StaticJsonDocument for ArduinoJson v6
    DeserializationError error = deserializeJson(doc, postBody);

    if (error) {
      Serial.print(F("[API] JSON parse failed for /schedule: "));
      Serial.println(error.f_str());
      request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}\n");
      return;
    }

    String applianceName = doc["appliance"].as<String>();
    JsonArray newSchedule = doc["schedule"].as<JsonArray>();
    bool clearOverride = doc["clear_override"] | false;

    if (applianceName.length() == 0) {
      request->send(400, "application/json", "{\"error\": \"Missing 'appliance' field\"}\n");
      return;
    }

    if (newSchedule.isNull()) {
      request->send(400, "application/json", "{\"error\": \"Missing 'schedule' array\"}\n");
      return;
    }

    // Verify appliance exists
    bool applianceExists = false;
    int applianceIndex = -1;
    for (int i = 0; i < NUM_APPLIANCES; i++) {
      if (appliances[i].name == applianceName) {
        applianceExists = true;
        applianceIndex = i;
        break;
      }
    }

    if (!applianceExists) {
      request->send(404, "application/json", "{\"error\": \"Appliance not found: " + applianceName + "\"}\n");
      return;
    }

    // Clear existing schedule for this appliance
    applianceSchedules[applianceName].clear();

    // Add new schedule entries
    for (JsonObject scheduleObj : newSchedule) {
      ScheduleEntry entry;
      entry.type = scheduleObj["type"].as<String>();
      // Convert hours and minutes to total minutes, or use direct minutes if provided
      if (scheduleObj["start_min"].is<int>()) {
        entry.start_min = scheduleObj["start_min"].as<int>();
        entry.end_min = scheduleObj["end_min"].as<int>();
      } else {
        // Legacy format conversion
        entry.start_min = scheduleObj["start_h"].as<int>() * 60 + scheduleObj["start_m"].as<int>();
        entry.end_min = scheduleObj["end_h"].as<int>() * 60 + scheduleObj["end_m"].as<int>();
      }
      applianceSchedules[applianceName].push_back(entry);
    }

    // Optional: Clear override for this appliance
    if (clearOverride && applianceIndex >= 0) {
      appliances[applianceIndex].overrideEndTime = 0;
      appliances[applianceIndex].overrideState = OFF;
      Serial.printf("[API] Override cleared for %s\n", applianceName.c_str());
    }

    // Save updated schedules to NVS
    markSchedulesDirty();  // Schedule delayed save

    // Immediately re-evaluate the specific appliance with new schedule
    DateTime now = rtc.now();
    int currentMinutes = now.hour() * 60 + now.minute();
    if (applianceIndex >= 0) {
      applyApplianceLogic(appliances[applianceIndex], currentMinutes);
    }

    Serial.printf("[API] Schedule updated for %s\n", applianceName.c_str());
    request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Schedule updated for " + applianceName + " and applied immediately\"}\n");
  }
}
