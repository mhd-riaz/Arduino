// ============================================================================
// ESP32 Fish Tank Automation System (Arduino Sketch)
// Version: 1.5 - LEDC API Fix (Based on official Espressif documentation)
// Author: mhd-riaz
// Date: June 2, 2025
//
// Description:
// This program automates a fishtank environment using an ESP32 Dev Module.
// It controls four appliances (Motor, CO2, Light, Heater) based on predefined
// schedules, real-time temperature, and user-initiated overrides via a REST API.
//
// Features:
// - Control of 4 appliances (Motor, CO2, Light, Heater).
// - DS1307 RTC for accurate timekeeping, synchronized with NTP.
// - DS18B20 temperature sensor for heater control.
// - 0.96" Blue OLED Display for status (time, appliance states).
// - 12V 8-channel optocoupler relay for appliance control.
// - ESP32 WiFi for network connectivity and NTP.
// - REST API server with JSON input/output for schedule overrides,
//   temporary appliance control, and WiFi configuration.
// - Non-Volatile Storage (NVS) for WiFi credentials and appliance schedules.
// - Temperature-controlled heater logic (25-29°C with 30 min min-run).
// - Fault tolerance for WiFi network issues (RTC fallback, non-blocking ops).
// - Buzzer feedback for WiFi connection setup issues.
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
//   - IN1 (Motor): GPIO 16 (example pin)
//   - IN2 (CO2): GPIO 17 (example pin)
//   - IN3 (Light): GPIO 18 (example pin)
//   - IN4 (Heater): GPIO 19 (example pin)
//   - Note: Relays are typically Active LOW.
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
// - driver/ledc.h (for ledcSetup, ledcAttachPin, ledcWrite, ledcWriteTone)
//
// Important Notes:
// - Ensure `Preferences.h` is used for NVS.
// - All timing operations use `millis()` for non-blocking behavior.
// - Relays are assumed to be Active LOW (LOW = ON, HIGH = OFF).
// - Heater control prioritizes temperature over schedule/override.
// - `ArduinoJson` buffer size must be sufficient for schedules.
// - Default schedules are loaded if NVS is empty.
// - The system attempts to connect to saved WiFi. If unsuccessful, it
//   starts an Access Point (AP) for configuration and buzzes.
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
// 2. Pin Definitions
// ============================================================================
// Appliance Relay Pins (Adjust these based on your wiring)
// Relays are assumed to be Active LOW: LOW = ON, HIGH = OFF
#define MOTOR_RELAY_PIN 16
#define CO2_RELAY_PIN 17
#define LIGHT_RELAY_PIN 18
#define HEATER_RELAY_PIN 19

// DS18B20 Temperature Sensor Pin
#define ONE_WIRE_BUS 14  // DS18B20 data pin

// Buzzer Pin (PWM capable)
#define BUZZER_PIN 13
#define BUZZER_CHANNEL 0  // LEDC channel for buzzer PWM

// OLED Display (I2C)
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)
#define I2C_ADDRESS 0x3C  // Or 0x3D for some 128x64 displays

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

// Time Management
RTC_DS1307 rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 5.5 * 3600, 60000);  // IST offset (5.5 hours), update interval 60s
unsigned long lastNtpSyncMillis = 0;
const unsigned long NTP_SYNC_INTERVAL_MS = 3 * 3600 * 1000;  // Sync every 3 hours

// Temperature Sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float currentTemperatureC = 0.0;
unsigned long lastTempReadMillis = 0;
const unsigned long TEMP_READ_INTERVAL_MS = 5000;  // Read temperature every 5 seconds

// Heater Control Logic
const float TEMP_THRESHOLD_ON = 25.0;                         // Turn heater on if temp drops below this
const float TEMP_THRESHOLD_OFF = 29.0;                        // Turn heater off if temp reaches/exceeds this
const unsigned long HEATER_MIN_RUN_TIME_MS = 30 * 60 * 1000;  // 30 minutes minimum run time
unsigned long heaterOnTimeMillis = 0;                         // Tracks when heater was turned on for min run time
bool heaterForcedOn = false;                                  // Flag to indicate if heater is forced on by temperature logic

// Appliance Management
enum ApplianceState { OFF,
                      ON };
enum ApplianceMode { SCHEDULED,
                     OVERRIDDEN,
                     TEMP_CONTROLLED };  // For heater, TEMP_CONTROLLED takes precedence

struct ScheduleEntry {
  String type;  // "on_interval" or "off_interval"
  int start_h;
  int start_m;
  int end_h;
  int end_m;
};

struct Appliance {
  String name;
  int pin;
  ApplianceState currentState;
  ApplianceState scheduledState;  // State dictated by schedule
  ApplianceState overrideState;   // State dictated by manual override
  unsigned long overrideEndTime;  // millis() when override expires (0 if no override)
  ApplianceMode currentMode;      // How the appliance is currently controlled
};

// Array to hold appliance objects
Appliance appliances[] = {
  { "Motor", MOTOR_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },
  { "CO2", CO2_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },
  { "Light", LIGHT_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },
  { "Heater", HEATER_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED }
};
const int NUM_APPLIANCES = sizeof(appliances) / sizeof(appliances[0]);

// Map to store schedules for each appliance
std::map<String, std::vector<ScheduleEntry>> applianceSchedules;

// OLED Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
unsigned long lastOLEDUpdateMillis = 0;
const unsigned long OLED_UPDATE_INTERVAL_MS = 1000;  // Update OLED every 1 second

// WiFi Connection Management
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 30000;  // 30 seconds for WiFi connection
bool wifiConnected = false;
bool apModeActive = false;

// ============================================================================
// 4. Function Prototypes
// ============================================================================
void connectWiFi();
void startAPMode();
void buzz(int count, int delayMs);
void syncTimeNTP();
void loadSchedules();
void saveSchedules();
void applyApplianceLogic(Appliance &app, DateTime now);
void updateOLED(DateTime now);
bool isTimeInInterval(int currentH, int currentM, int startH, int startM, int endH, int endM);
bool authenticateRequest(AsyncWebServerRequest *request);

// REST API Handlers
void handleRoot(AsyncWebServerRequest *request);
void handleStatus(AsyncWebServerRequest *request);
void handleControl(AsyncWebServerRequest *request);
void handleGetSchedules(AsyncWebServerRequest *request);
void handlePostSchedules(AsyncWebServerRequest *request);
void handleWifiConfig(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);

// ============================================================================
// 5. Setup Function
// ============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n[SETUP] Starting ESP32 Fish Tank Automation System...");

  // Initialize NVS
  if (!preferences.begin(NVS_NAMESPACE_WIFI, false)) {
    Serial.println("[NVS] Failed to initialize WiFi preferences.");
  }
  if (!preferences.begin(NVS_NAMESPACE_SCHEDULES, false)) {
    Serial.println("[NVS] Failed to initialize schedule preferences.");
  }

  // Initialize I2C bus
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println("[I2C] I2C bus initialized.");

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("[RTC] Couldn't find RTC! Check wiring.");
    while (1)
      ;  // Halt if RTC is not found, it's critical
  }
  if (!rtc.isrunning()) {
    Serial.println("[RTC] RTC is NOT running, setting time from build time.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set to compile time as fallback
  }
  Serial.println("[RTC] RTC initialized.");

  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
    Serial.println("[OLED] SSD1306 allocation failed. Check wiring/address.");
    // Continue without display if failed, but log error
  } else {
    Serial.println("[OLED] Display initialized.");
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
  Serial.println("[DS18B20] Temperature sensor initialized.");

  // Configure Relay Pins and ensure they are OFF initially (Active HIGH for safety)
  for (int i = 0; i < NUM_APPLIANCES; i++) {
    pinMode(appliances[i].pin, OUTPUT);
    digitalWrite(appliances[i].pin, HIGH);  // Set HIGH to ensure relays are OFF (Active LOW)
    Serial.printf("[RELAY] Pin %d (%s) set to OFF.\n", appliances[i].pin, appliances[i].name.c_str());
  }

  // Configure Buzzer Pin using corrected LEDC API
  ledcAttachChannel(BUZZER_PIN, 1000, 8, BUZZER_CHANNEL);  // pin, frequency, resolution, channel
  ledcWrite(BUZZER_CHANNEL, 0);                            // Ensure buzzer is off (0% duty cycle)
  Serial.println("[BUZZER] Buzzer configured.");

  // Attempt WiFi Connection
  connectWiFi();

  // If WiFi connection failed, start AP mode and buzz
  if (!wifiConnected) {
    Serial.println("[WiFi] Failed to connect to saved WiFi. Starting AP mode...");
    buzz(3, 1000);  // Buzz thrice with 1000ms delay
    startAPMode();
  } else {
    Serial.println("[WiFi] Connected to WiFi.");
    // Sync RTC with NTP immediately after successful WiFi connection
    syncTimeNTP();
  }

  // Load schedules from NVS
  loadSchedules();

  // Configure REST API Endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/schedules", HTTP_GET, handleGetSchedules);
  server.on("/schedules", HTTP_POST, handlePostSchedules);
  server.on("/wifi", HTTP_POST, handleWifiConfig);
  server.onNotFound(handleNotFound);

  // Start the server
  server.begin();
  Serial.println("[API] REST API server started.");

  Serial.println("[SETUP] System setup complete.");
}

// ============================================================================
// 6. Loop Function
// ============================================================================
void loop() {
  // Handle WiFi reconnection if disconnected
  if (WiFi.status() != WL_CONNECTED && !apModeActive) {
    if (millis() - lastNtpSyncMillis > 5000) {  // Check every 5 seconds
      Serial.println("[WiFi] Reconnecting to WiFi...");
      connectWiFi();
      lastNtpSyncMillis = millis();  // Use this timer for reconnection attempts
    }
  }

  // Periodically sync RTC with NTP (every 3 hours, if WiFi available)
  if (wifiConnected && (millis() - lastNtpSyncMillis >= NTP_SYNC_INTERVAL_MS)) {
    syncTimeNTP();
    lastNtpSyncMillis = millis();
  }

  // Get current time from RTC
  DateTime now = rtc.now();

  // Read temperature periodically
  if (millis() - lastTempReadMillis >= TEMP_READ_INTERVAL_MS) {
    sensors.requestTemperatures();
    currentTemperatureC = sensors.getTempCByIndex(0);
    if (currentTemperatureC == DEVICE_DISCONNECTED_C) {
      Serial.println("[DS18B20] Error reading temperature. Sensor disconnected?");
      currentTemperatureC = 0.0;  // Reset to 0 or last valid value
    } else {
      Serial.printf("[DS18B20] Temperature: %.2f C\n", currentTemperatureC);
    }
    lastTempReadMillis = millis();
  }

  // Apply appliance control logic for each appliance
  for (int i = 0; i < NUM_APPLIANCES; i++) {
    applyApplianceLogic(appliances[i], now);
  }

  // Update OLED display periodically
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
  String ssid = preferences.getString(NVS_KEY_SSID, "");
  String password = preferences.getString(NVS_KEY_PASS, "");

  if (ssid.length() == 0) {
    Serial.println("[WiFi] No saved WiFi credentials. Cannot connect to STA mode.");
    wifiConnected = false;
    return;
  }

  Serial.printf("[WiFi] Attempting to connect to SSID: %s\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Connecting WiFi...");
    display.setCursor(0, 10);
    display.println(ssid);
    display.display();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected to WiFi!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
    apModeActive = false;  // Ensure AP mode is off if STA connected
  } else {
    Serial.println("\n[WiFi] Failed to connect to WiFi.");
    wifiConnected = false;
  }
}

/**
 * @brief Starts the ESP32 in Access Point (AP) mode for configuration.
 */
void startAPMode() {
  Serial.println("[AP] Starting Access Point mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Fishtank_Setup", "password123");  // Default AP name and password
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("[AP] AP IP address: ");
  Serial.println(apIP);
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
  Serial.printf("[BUZZER] Buzzing %d times...\n", count);
  for (int i = 0; i < count; i++) {
    ledcWriteTone(BUZZER_CHANNEL, 1000);  // 1kHz tone (using original LEDC API)
    delay(200);                           // Buzz duration
    ledcWriteTone(BUZZER_CHANNEL, 0);     // Turn off (using original LEDC API)
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
    Serial.println("[NTP] Syncing time with NTP server...");
    timeClient.begin();
    if (timeClient.forceUpdate()) {
      unsigned long epochTime = timeClient.getEpochTime();
      rtc.adjust(DateTime(epochTime));
      Serial.printf("[NTP] RTC adjusted to: %s\n", rtc.now().timestamp().c_str());
      timeClient.end();  // End NTP client to free resources
    } else {
      Serial.println("[NTP] Failed to get time from NTP server.");
    }
  } else {
    Serial.println("[NTP] WiFi not connected, skipping NTP sync.");
  }
}

/**
 * @brief Loads appliance schedules from NVS. If no schedules are found,
 * default schedules are used and saved to NVS.
 */
void loadSchedules() {
  String schedulesJson = preferences.getString(NVS_KEY_SCHEDULES, "");

  if (schedulesJson.length() == 0) {
    Serial.println("[NVS] No schedules found in NVS. Loading default schedules.");
    // Define default schedules
    applianceSchedules["Motor"].push_back({ "off_interval", 9, 30, 10, 30 });
    applianceSchedules["CO2"].push_back({ "on_interval", 7, 0, 14, 0 });
    applianceSchedules["CO2"].push_back({ "on_interval", 15, 0, 21, 0 });
    applianceSchedules["Light"].push_back({ "on_interval", 10, 0, 14, 0 });
    applianceSchedules["Light"].push_back({ "on_interval", 15, 30, 19, 30 });
    applianceSchedules["Heater"].push_back({ "on_interval", 0, 0, 4, 0 });
    applianceSchedules["Heater"].push_back({ "on_interval", 3, 30, 17, 0 });

    saveSchedules();  // Save default schedules to NVS
    return;
  }

  Serial.println("[NVS] Loading schedules from NVS...");
  JsonDocument doc;  // Changed from StaticJsonDocument to JsonDocument
  DeserializationError error = deserializeJson(doc, schedulesJson);

  if (error) {
    Serial.print(F("[NVS] deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  applianceSchedules.clear();  // Clear existing schedules before loading new ones

  for (JsonPair p : doc.as<JsonObject>()) {
    String applianceName = p.key().c_str();
    JsonArray schedulesArray = p.value().as<JsonArray>();

    for (JsonObject scheduleObj : schedulesArray) {
      ScheduleEntry entry;
      entry.type = scheduleObj["type"].as<String>();
      entry.start_h = scheduleObj["start_h"].as<int>();
      entry.start_m = scheduleObj["start_m"].as<int>();
      entry.end_h = scheduleObj["end_h"].as<int>();
      entry.end_m = scheduleObj["end_m"].as<int>();
      applianceSchedules[applianceName].push_back(entry);
    }
  }
  Serial.println("[NVS] Schedules loaded successfully.");
}

/**
 * @brief Saves current appliance schedules to NVS.
 */
void saveSchedules() {
  JsonDocument doc;  // Changed from StaticJsonDocument to JsonDocument

  for (auto const &[applianceName, schedulesVec] : applianceSchedules) {
    // Use doc[key].to<JsonArray>() for modern ArduinoJson
    JsonArray schedulesArray = doc[applianceName].to<JsonArray>();
    for (const auto &entry : schedulesVec) {
      JsonObject scheduleObj = schedulesArray.add<JsonObject>();
      scheduleObj["type"] = entry.type;
      scheduleObj["start_h"] = entry.start_h;
      scheduleObj["start_m"] = entry.start_m;
      scheduleObj["end_h"] = entry.end_h;
      scheduleObj["end_m"] = entry.end_m;
    }
  }

  String output;
  serializeJson(doc, output);
  preferences.putString(NVS_KEY_SCHEDULES, output);
  Serial.println("[NVS] Schedules saved to NVS.");
  Serial.println(output);  // For debugging
}

/**
 * @brief Applies the control logic for a single appliance, considering schedules,
 * overrides, and temperature (for heater).
 * @param app Reference to the Appliance object.
 * @param now Current DateTime from RTC.
 */
void applyApplianceLogic(Appliance &app, DateTime now) {
  ApplianceState targetState = OFF;  // Default to OFF
  app.currentMode = SCHEDULED;       // Default mode

  // 1. Check for active override
  if (app.overrideEndTime > 0) {
    if (millis() < app.overrideEndTime) {
      targetState = app.overrideState;
      app.currentMode = OVERRIDDEN;
      Serial.printf("[%s] Override active. State: %s, Remaining: %lu ms\n",
                    app.name.c_str(), (targetState == ON ? "ON" : "OFF"), (app.overrideEndTime - millis()));
    } else {
      // Override expired
      app.overrideEndTime = 0;
      Serial.printf("[%s] Override expired. Reverting to schedule.\n", app.name.c_str());
    }
  }

  // 2. Apply Scheduled Logic (only if not overridden or override expired)
  if (app.overrideEndTime == 0) {
    if (applianceSchedules.count(app.name)) {
      std::vector<ScheduleEntry> &schedules = applianceSchedules[app.name];
      bool isScheduledOn = false;
      bool isScheduledOff = false;

      for (const auto &schedule : schedules) {
        if (isTimeInInterval(now.hour(), now.minute(), schedule.start_h, schedule.start_m, schedule.end_h, schedule.end_m)) {
          if (schedule.type == "on_interval") {
            isScheduledOn = true;
          } else if (schedule.type == "off_interval") {
            isScheduledOff = true;
          }
        }
      }

      if (app.name == "Motor") {
        // Motor: OFF only from 9:30 to 10:30, rest ON
        targetState = isScheduledOff ? OFF : ON;
      } else {
        // CO2, Light, Heater (scheduled part): ON during specified intervals
        targetState = isScheduledOn ? ON : OFF;
      }
    } else {
      // No schedules defined for this appliance, default to OFF
      targetState = OFF;
    }
    app.scheduledState = targetState;  // Store the state dictated by schedule
  }

  // 3. Heater Specific Logic (highest priority)
  if (app.name == "Heater") {
    if (currentTemperatureC > 0) {  // Only apply if temperature is valid
      if (currentTemperatureC < TEMP_THRESHOLD_ON) {
        if (app.currentState == OFF && !heaterForcedOn) {  // Only set heaterOnTimeMillis if turning ON
          heaterOnTimeMillis = millis();
          heaterForcedOn = true;
          Serial.println("[Heater] Temp too low. FORCING ON. Starting min run timer.");
        }
        targetState = ON;
        app.currentMode = TEMP_CONTROLLED;
      } else if (currentTemperatureC >= TEMP_THRESHOLD_OFF) {
        if (heaterForcedOn && (millis() - heaterOnTimeMillis < HEATER_MIN_RUN_TIME_MS)) {
          // If forced on and min run time not met, keep it ON
          targetState = ON;
          app.currentMode = TEMP_CONTROLLED;
          Serial.printf("[Heater] Temp high, but min run time (%lu ms) not met. Keeping ON.\n", HEATER_MIN_RUN_TIME_MS - (millis() - heaterOnTimeMillis));
        } else {
          // Temp high AND min run time met (or not forced on) -> turn OFF
          heaterForcedOn = false;
          heaterOnTimeMillis = 0;
          targetState = OFF;
          app.currentMode = (app.overrideEndTime > 0) ? OVERRIDDEN : SCHEDULED;  // Revert to previous mode
          Serial.println("[Heater] Temp reached target or min run time met. Turning OFF.");
        }
      }
      // If temperature is between thresholds, heater state depends on previous state
      // If it was forced ON, it stays ON until min_run_time or temp_off_threshold is met.
      // If it was OFF, it stays OFF.
      if (heaterForcedOn && targetState == OFF) {  // If heater was forced on, but now schedule/override says off, keep it on until min_run_time or temp_off is met
        targetState = ON;
        app.currentMode = TEMP_CONTROLLED;
      }
    }
  }

  // Update appliance state if changed
  if (app.currentState != targetState) {
    app.currentState = targetState;
    digitalWrite(app.pin, (app.currentState == ON ? LOW : HIGH));  // Active LOW relay
    Serial.printf("[%s] State changed to: %s (Mode: %s)\n",
                  app.name.c_str(), (app.currentState == ON ? "ON" : "OFF"),
                  (app.currentMode == SCHEDULED ? "SCHEDULED" : (app.currentMode == OVERRIDDEN ? "OVERRIDDEN" : "TEMP_CONTROLLED")));
  }
}

/**
 * @brief Updates the OLED display with current time and appliance states.
 * @param now Current DateTime from RTC.
 */
void updateOLED(DateTime now) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Row 1: Current Time
  display.setCursor(0, 0);
  char timeStr[20];
  sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d",
          now.hour(), now.minute(), now.second(),
          now.day(), now.month(), now.year());
  display.println(timeStr);

  // Row 2 onwards: Appliance States
  int y_offset = 10;
  for (int i = 0; i < NUM_APPLIANCES; i++) {
    display.setCursor(0, y_offset + (i * 8));
    display.printf("%s: %s (%s)",
                   appliances[i].name.c_str(),
                   (appliances[i].currentState == ON ? "ON" : "OFF"),
                   (appliances[i].currentMode == SCHEDULED ? "S" : (appliances[i].currentMode == OVERRIDDEN ? "O" : "T")));
    // Display remaining override time if active
    if (appliances[i].overrideEndTime > 0 && appliances[i].currentMode == OVERRIDDEN) {
      unsigned long remainingSec = (appliances[i].overrideEndTime - millis()) / 1000;
      display.printf(" [%lu s]", remainingSec);
    }
  }

  // Display Temperature
  display.setCursor(0, y_offset + (NUM_APPLIANCES * 8));
  display.printf("Temp: %.1f C", currentTemperatureC);

  // Display WiFi Status
  display.setCursor(0, y_offset + (NUM_APPLIANCES * 8) + 8);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("WiFi: Connected");
  } else if (apModeActive) {
    display.print("WiFi: AP Mode");
  } else {
    display.print("WiFi: Disconnected");
  }

  display.display();
}

/**
 * @brief Checks if a given time (currentH, currentM) falls within a specified
 * interval (startH:startM to endH:endM). Handles overnight intervals.
 * @param currentH Current hour.
 * @param currentM Current minute.
 * @param startH Start hour of interval.
 * @param startM Start minute of interval.
 * @param endH End hour of interval.
 * @param endM End minute of interval.
 * @return True if time is in interval, false otherwise.
 */
bool isTimeInInterval(int currentH, int currentM, int startH, int startM, int endH, int endM) {
  int currentTimeInMinutes = currentH * 60 + currentM;
  int startTimeInMinutes = startH * 60 + startM;
  int endTimeInMinutes = endH * 60 + endM;

  if (startTimeInMinutes <= endTimeInMinutes) {
    // Normal interval (e.g., 07:00 - 14:00)
    return (currentTimeInMinutes >= startTimeInMinutes && currentTimeInMinutes < endTimeInMinutes);
  } else {
    // Overnight interval (e.g., 22:00 - 06:00)
    return (currentTimeInMinutes >= startTimeInMinutes || currentTimeInMinutes < endTimeInMinutes);
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
      app_status["timeout_seconds"] = (appliances[i].overrideEndTime - millis()) / 1000;
    }
  }

  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

/**
 * @brief Handles the "/control" POST endpoint. Allows overriding appliance states.
 * JSON Input:
 * {
 * "api_key": "Automate@123", // Provided in header, but can also be in body for testing
 * "appliance": "Motor",
 * "action": "ON", // or "OFF"
 * "timeout_minutes": 60 // Optional
 * }
 * OR
 * {
 * "api_key": "Automate@123",
 * "appliances": [
 * {"name": "CO2", "action": "OFF", "timeout_minutes": 30},
 * {"name": "Light", "action": "ON"}
 * ]
 * }
 */
void handleControl(AsyncWebServerRequest *request) {
  if (!authenticateRequest(request)) return;

  if (request->method() == HTTP_POST) {
    if (request->hasParam("plain", true)) {
      String requestBody = request->getParam("plain", true)->value();
      JsonDocument doc;  // Changed from StaticJsonDocument to JsonDocument
      DeserializationError error = deserializeJson(doc, requestBody);

      if (error) {
        Serial.print(F("[API] deserializeJson() failed for /control: "));
        Serial.println(error.f_str());
        request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}\n");
        return;
      }

      JsonArray appliancesArray = doc["appliances"].as<JsonArray>();

      if (appliancesArray.isNull()) {  // Single appliance control
        String appName = doc["appliance"].as<String>();
        String action = doc["action"].as<String>();
        int timeoutMinutes = doc["timeout_minutes"] | 0;  // Default to 0 (no timeout)

        bool found = false;
        for (int i = 0; i < NUM_APPLIANCES; i++) {
          if (appliances[i].name == appName) {
            found = true;
            appliances[i].overrideState = (action == "ON" ? ON : OFF);
            appliances[i].overrideEndTime = (timeoutMinutes > 0) ? (millis() + (unsigned long)timeoutMinutes * 60 * 1000) : 0;
            Serial.printf("[API] Appliance %s override to %s for %d mins.\n", appName.c_str(), action.c_str(), timeoutMinutes);
            break;
          }
        }
        if (!found) {
          request->send(404, "application/json", "{\"error\": \"Appliance not found\"}\n");
          return;
        }
      } else {  // Multiple appliances control
        for (JsonObject appObj : appliancesArray) {
          String appName = appObj["name"].as<String>();
          String action = appObj["action"].as<String>();
          int timeoutMinutes = appObj["timeout_minutes"] | 0;

          bool found = false;
          for (int i = 0; i < NUM_APPLIANCES; i++) {
            if (appliances[i].name == appName) {
              found = true;
              appliances[i].overrideState = (action == "ON" ? ON : OFF);
              appliances[i].overrideEndTime = (timeoutMinutes > 0) ? (millis() + (unsigned long)timeoutMinutes * 60 * 1000) : 0;
              Serial.printf("[API] Appliance %s override to %s for %d mins.\n", appName.c_str(), action.c_str(), timeoutMinutes);
              break;
            }
          }
          if (!found) {
            Serial.printf("[API] Appliance %s not found in request.\n", appName.c_str());
          }
        }
      }
      request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Appliance control updated\"}\n");
    } else {
      request->send(400, "application/json", "{\"error\": \"Missing JSON body\"}\n");
    }
  } else {
    request->send(405, "application/json", "{\"error\": \"Method Not Allowed\"}\n");
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
      scheduleObj["start_h"] = entry.start_h;
      scheduleObj["start_m"] = entry.start_m;
      scheduleObj["end_h"] = entry.end_h;
      scheduleObj["end_m"] = entry.end_m;
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
void handlePostSchedules(AsyncWebServerRequest *request) {
  if (!authenticateRequest(request)) return;

  if (request->method() == HTTP_POST) {
    if (request->hasParam("plain", true)) {
      String requestBody = request->getParam("plain", true)->value();
      JsonDocument doc;  // Changed from StaticJsonDocument to JsonDocument
      DeserializationError error = deserializeJson(doc, requestBody);

      if (error) {
        Serial.print(F("[API] deserializeJson() failed for /schedules: "));
        Serial.println(error.f_str());
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
          entry.start_h = scheduleObj["start_h"].as<int>();
          entry.start_m = scheduleObj["start_m"].as<int>();
          entry.end_h = scheduleObj["end_h"].as<int>();
          entry.end_m = scheduleObj["end_m"].as<int>();
          applianceSchedules[applianceName].push_back(entry);
        }
      }

      saveSchedules();  // Save the new schedules to NVS
      request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Schedules updated and saved to NVS\"}\n");

    } else {
      request->send(400, "application/json", "{\"error\": \"Missing JSON body\"}\n");
    }
  } else {
    request->send(405, "application/json", "{\"error\": \"Method Not Allowed\"}\n");
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
void handleWifiConfig(AsyncWebServerRequest *request) {
  if (!authenticateRequest(request)) return;

  if (request->method() == HTTP_POST) {
    if (request->hasParam("plain", true)) {
      String requestBody = request->getParam("plain", true)->value();
      JsonDocument doc;  // Changed from StaticJsonDocument to JsonDocument
      DeserializationError error = deserializeJson(doc, requestBody);

      if (error) {
        Serial.print(F("[API] deserializeJson() failed for /wifi: "));
        Serial.println(error.f_str());
        request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}\n");
        return;
      }

      String newSsid = doc["ssid"].as<String>();
      String newPassword = doc["password"].as<String>();

      if (newSsid.length() > 0) {
        preferences.putString(NVS_KEY_SSID, newSsid);
        preferences.putString(NVS_KEY_PASS, newPassword);
        Serial.printf("[NVS] New WiFi credentials saved: SSID=%s\n", newSsid.c_str());
        request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"WiFi credentials saved. Rebooting to connect...\"}\n");
        delay(1000);    // Give time for response to send
        ESP.restart();  // Soft reboot to connect to new WiFi
      } else {
        request->send(400, "application/json", "{\"error\": \"SSID cannot be empty\"}\n");
      }
    } else {
      request->send(400, "application/json", "{\"error\": \"Missing JSON body\"}\n");
    }
  } else {
    request->send(405, "application/json", "{\"error\": \"Method Not Allowed\"}\n");
  }
}

/**
 * @brief Handles requests for unknown endpoints.
 */
void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not Found");
}
