// ============================================================================
// ESP32 Fish Tank Automation System (Arduino Sketch)
// Version: 3.0 - Smart Fish Tank Controller
// Author: mhd-riaz
// Date: September 28, 2025
//
// What this program does:
// This is a smart system that automatically controls a fish tank!
// It can turn lights on/off, run filters, add CO2 for plants, and control heater temperature.
// It uses WiFi to let you control everything from your phone or computer.
//
// Main Features:
// - Controls 5 things: Filter, CO2, Light, Heater, and extra Filter
// - Uses a clock (RTC) to know what time it is
// - Measures water temperature with a special sensor
// - Connects to WiFi for remote control
// - Shows status on a small screen (OLED)
// - Has a REST API for phone/computer control
// - Saves settings even when power is off
// - Makes beeps to tell you what's happening
// - Has safety features to protect your fish
//
// How it works:
// 1. Connects to your WiFi (or creates its own hotspot)
// 2. Reads the current time from the clock module
// 3. Checks water temperature
// 4. Decides what to turn on/off based on time and temperature
// 5. Shows status on the screen
// 6. Lets you control it remotely via web requests
//
// Hardware needed:
// - ESP32 microcontroller (the brain)
// - DS3231 clock module (to keep time)
// - DS18B20 temperature sensor (to measure water temp)
// - SSD1306 OLED screen (to show information)
// - 8-channel relay module (to control power to devices)
// - Buzzer (to make sounds)
// - Power supplies (5V and 12V)
//
// Safety Note:
// This system controls electricity! Always be careful with power connections.
// Have an adult help you, and never work on live electrical circuits.
//
// ============================================================================
// TABLE OF CONTENTS (What Each Part Does)
// ============================================================================
//
// 1. SAFETY INFO ..................... Lines 102-210
//    - Explains electrical safety and protection
//    - Lists all the parts you need
//    - Tells you how to wire everything safely
//
// 2. LIBRARIES ....................... Lines 212-248
//    - Includes code from other smart people
//    - These libraries do the hard work for us
//
// 3. SETTINGS & RULES ................ Lines 250-318
//    - Defines all the pins and connections
//    - Sets up timing and temperature rules
//    - Configures how everything works
//
// 4. DATA STRUCTURES ................ Lines 320-448
//    - Defines what information we store
//    - Creates templates for our data
//    - Sets up variables we use throughout
//
// 5. HELPER FUNCTIONS ................ Lines 450-548
//    - Small functions that do specific jobs
//    - Like turning relays on/off safely
//
// 6. SETUP FUNCTION .................. Lines 549-748
//    - Runs once when the system starts
//    - Initializes all hardware and connections
//    - Gets everything ready to work
//
// 7. MAIN LOOP ....................... Lines 750-848
//    - Runs forever while system is on
//    - Checks temperature, time, and controls devices
//    - Updates the screen and handles web requests
//
// 8. WIFI MANAGEMENT ................ Lines 850-1098
//    - Connects to your home WiFi
//    - Creates hotspot if WiFi fails
//    - Handles network communication
//
// 9. WEB API ENDPOINTS .............. Lines 1100-1298
//    - Responds to web requests from phones/computers
//    - Lets you control the system remotely
//
// 10. DEVICE CONTROL ................ Lines 1300-1498
//    - Decides when to turn devices on/off
//    - Handles schedules and temperature control
//
// 11. TEMPERATURE SYSTEM ............ Lines 1500-1698
//    - Reads temperature sensor
//    - Controls the heater automatically
//
// 12. SCREEN DISPLAY ................ Lines 1700-1898
//    - Shows information on the OLED screen
//    - Updates time, temperature, and device status
//
// 13. SAFETY SYSTEMS ................ Lines 1900-1998
//    - Protects fish if temperature is too high/low
//    - Handles emergency situations
//
// 14. SCHEDULE MANAGEMENT ........... Lines 2000-end
//    - Saves and loads device schedules
//    - Lets you customize when things turn on/off
//
// ============================================================================
// SAFETY FIRST! ⚠️ ELECTRICAL SAFETY INFORMATION
// ============================================================================
//
// IMPORTANT: This project uses electricity! Always be careful and get help from an adult.
//
// Why safety matters:
// - Fish tanks have water, and water + electricity = DANGER!
// - We need to protect you, your fish, and your home
// - Always follow these safety rules
//
// What you need (provided by customer/adult):
// 1. Main Power Breaker: 6A MCB (like a safety switch for your home)
// 2. Power Supplies: 5V charger (like phone charger) + 12V power supply
// 3. Surge Protector: Protects against lightning and power spikes
// 4. Ground Fault Circuit Interrupter (GFCI): Shuts off if there's a problem
//
// Our PCB (Printed Circuit Board) has built-in safety:
// - Fuses: Like circuit breakers that burn out to protect circuits
// - Diodes: One-way valves for electricity to prevent damage
// - Resistors: Control how much electricity flows
// - Filters: Clean up electrical noise
//
// How electricity flows safely:
// House Power (240V) → Breaker → Power Supplies → Our PCB → Devices
//                    ↓
//              Surge Protection
//
// ============================================================================
// HARDWARE PROTECTION COMPONENTS SHOPPING LIST:
// ============================================================================
//
// FUSES & CIRCUIT BREAKERS:
// - 6A MCB (C-curve, 6kA): Main AC supply protection
// - T3AL250V: 3A slow-blow ceramic fuse (12V rail)
// - T2.5AL250V: 2.5A slow-blow ceramic fuse (5V rail)
// - F1AL250V: 1A fast-blow fuse (ESP32 VIN)
// - PTC Fuses: 100mA, 200mA self-resetting (sensor lines)
//
// FUSES & CIRCUIT BREAKERS:
// - 6A MCB (C-curve, 6kA): Main AC supply protection - Customer provides
// - T3AL250V: 3A slow-blow ceramic fuse (12V rail) - Built into PCB
// - T2.5AL250V: 2.5A slow-blow ceramic fuse (5V rail) - Built into PCB
//
// SURGE PROTECTION DIODES:
// - 1N4744A: 15V Zener diode (5V and 12V rail protection) - Built into PCB
// - 1N5819: 3A/40V Schottky (reverse polarity protection) - Built into PCB
//
// CAPACITORS (POWER FILTERING):
// - 470µF/25V: Electrolytic capacitors (C1, C3, C5) - Built into PCB
//
// EMI SUPPRESSION:
// - BLM18PG221SN1D: Ferrite bead (EMI suppression, 220Ω@100MHz) - Built into PCB
// - Alternative: BLM21PG221SN1D or MMZ1608Y121BT000 (if unavailable)
//
// RESISTORS (SIGNAL CONDITIONING):
// - 330Ω/0.25W: Current limiting resistors (R4-R8, GPIO to relays) - Built into PCB
// - 4.7kΩ/0.25W: Pull-up resistors (R1-R3, I2C and OneWire) - Built into PCB
//
// CONNECTORS & MECHANICAL:
// - Phoenix Contact screw terminals: Power input connections - Built into PCB
// - JST XH connectors: Module connections (I2C, sensors) - Built into PCB
// - IP65 Enclosure: Waterproof housing for electronics - Customer provides
//
// CUSTOMER INSTALLATION COMPONENTS:
// - 6A MCB: Main circuit breaker - Customer electrical panel
// - 275V MOV/SPD: Surge protection device - Customer electrical panel
// - SMPS 12V/2A: Relay power supply - Customer provides
// - ERD Charger 5V/2A: ESP32 power supply - Customer provides
// - Professional installation by qualified electrician - Customer responsibility
//
// Note: Always consult a qualified electrician for mains wiring.
//       Use proper isolation transformers for development/testing.
//       Never work on live circuits - always power off before modifications.
// ============================================================================

// ============================================================================
// 2. LIBRARIES - BORROWING SMART CODE FROM OTHERS
// ============================================================================
//
// Libraries are like toolboxes full of helpful code that other smart people wrote.
// Instead of writing everything from scratch, we "borrow" their code to make ours work.
// This is called "standing on the shoulders of giants" - using what others built.
//
// WiFi Library: Lets our ESP32 connect to the internet and WiFi networks
// Web Server Library: Creates a website that runs on our ESP32 for remote control
// Storage Library: Saves our settings even when power is turned off
// Clock Library: Helps us talk to the real-time clock module
// Temperature Library: Reads the water temperature sensor
// Screen Library: Controls the tiny OLED display screen
// JSON Library: Helps us send and receive data in a format computers understand
// Time Library: Gets accurate time from the internet
//
// These libraries do the hard technical work so we can focus on the fish tank logic!
//
// ============================================================================



// ============================================================================
// MEMORY OPTIMIZATION - SAVING COMPUTER BRAIN SPACE
// ============================================================================
//
// The ESP32 has limited memory (like a tiny brain with limited space).
// PROGMEM means "Program Memory" - we store text in flash memory instead of RAM.
// This saves the precious RAM for important things like calculations and data.
//
// Why this matters:
// - ESP32 has only about 320KB of RAM (working memory)
// - Flash memory has 4MB+ (permanent storage)
// - Text strings take up RAM space we need for other things
// - By storing strings in flash, we free up RAM for the program to work
//
// ============================================================================
// const char PROGMEM STR_OK[] = "OK";
// const char PROGMEM STR_ERROR[] = "Error";
// const char PROGMEM STR_SUCCESS[] = "Success";
// const char PROGMEM STR_INVALID[] = "Invalid";
// const char PROGMEM STR_JSON_PARSE_ERROR[] = "JSON parse error";
// const char PROGMEM STR_MISSING_PARAMS[] = "Missing required parameters";
// const char PROGMEM STR_INVALID_API_KEY[] = "Invalid API key";
// const char PROGMEM STR_APPLIANCE_NOT_FOUND[] = "Appliance not found";
const char PROGMEM STR_CONTENT_TYPE_JSON[] = "application/json";
// const char PROGMEM STR_ACCESS_CONTROL[] = "Access-Control-Allow-Origin";
// const char PROGMEM STR_WILDCARD[] = "*";


// ============================================================================
// 3. PIN DEFINITIONS - WHERE EVERYTHING CONNECTS
// ============================================================================
//
// The ESP32 has 38 pins (like little doors) that connect to different things.
// We tell the computer which pin connects to what device.
// This is like labeling wires so the computer knows where to send signals.
//
// Relay Pins (Control Power to Devices):
// - MOTOR_RELAY_PIN: Controls the main water filter pump
// - CO2_RELAY_PIN: Controls CO2 system for plants
// - LIGHT_RELAY_PIN: Controls aquarium lights
// - HEATER_RELAY_PIN: Controls water heater
// - HANGON_FILTER_PIN: Controls extra filter
//
// Sensor Pins:
// - ONE_WIRE_BUS: Connects to temperature sensor
//
// Output Pins:
// - BUZZER_PIN: Makes beeping sounds
//
// Display Pins (I2C):
// - OLED_SDA, OLED_SCL: Connect to the tiny screen
//
// ============================================================================
// 4. GLOBAL MEMORY BOXES - WHERE WE STORE INFORMATION
// ============================================================================
//
// Global variables are like boxes in the computer's memory where we store information.
// The computer needs to remember things like temperature, time, and device states.
// These boxes hold that information while the program runs.
//
// API Server: Handles web requests from phones/computers
// Storage System: Saves settings even when power is off
// Time System: Keeps track of current time (clock + internet time)
// Temperature System: Reads and remembers water temperature
// Heater Control: Smart logic to keep water at perfect temperature
// Device Control: Manages all the fish tank equipment
//
// ============================================================================

// ============================================================================
// 5. DATA STRUCTURES - ORGANIZING OUR INFORMATION
// ============================================================================
//
// Data structures are like organized boxes where we store related information together.
// Instead of having lots of separate variables, we group them into neat packages.
// This makes the code easier to understand and work with.
//
// ApplianceState: Simple ON/OFF choices for devices
// ApplianceMode: How the device is being controlled (schedule, manual, temperature)
// ScheduleEntry: Stores when a device should turn on/off
// Appliance: Complete information about each device (name, pin, states, etc.)
//
// Think of it like organizing your school supplies:
// - Pens in one box, pencils in another
// - Each box has a label and knows what's inside
//
// ============================================================================

// Array of 5 appliance objects with custom configurations
Appliance appliances[] = {
  { "Filter", MOTOR_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },         // Primary filtration (GPIO 18)
  { "CO2", CO2_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },              // CO2 injection system (GPIO 16)
  { "Light", LIGHT_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },          // LED lighting system (GPIO 5)
  { "Heater", HEATER_RELAY_PIN, OFF, OFF, OFF, 0, SCHEDULED },        // Water heater with temp control (GPIO 19)
  { "HangOnFilter", HANGON_FILTER_PIN, OFF, OFF, OFF, 0, SCHEDULED }  // Secondary hang-on filter (GPIO 17)
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

// System Health Monitoring & Alert System
// unsigned long lastSystemHeartbeat = 0;
// unsigned long lastLoopTime = 0;
// const unsigned long SYSTEM_WATCHDOG_TIMEOUT_MS = 90000;  // 90 seconds
// const unsigned long LOOP_HANG_TIMEOUT_MS = 45000;        // 45 seconds
// bool systemHealthOK = true;
// bool alertSounded = false;

// Schedule caching for performance
bool schedulesDirty = false;  // Flag to track if schedules need saving
unsigned long lastScheduleSaveMillis = 0;
const unsigned long SCHEDULE_SAVE_DELAY_MS = 5000;  // Save schedules 5 seconds after last change

// ============================================================================
// 6. FUNCTION BLUEPRINTS - WHAT EACH PART DOES
// ============================================================================
//
// Function prototypes are like a table of contents for the functions in our program.
// They tell the computer "these functions exist and here's what they need/do".
// This helps the computer understand the program before it reads all the details.
//
// Core Functions: Main jobs like connecting to WiFi and reading temperature
// Control Functions: Turning devices on/off and managing schedules
// Display Functions: Showing information on the screen
// Safety Functions: Protecting fish if something goes wrong
// Web Functions: Handling requests from phones/computers
//
// ============================================================================
void connectWiFi();
void startAPMode();
// void buzz(int count, int delayMs);
// void buzzPattern(int pattern);
void syncTimeNTP();

// System Health Monitoring
// void updateSystemHeartbeat();
// void checkSystemHealth();
// void alertSystemFailure(const char *reason);

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
// void handleForceRefresh(AsyncWebServerRequest *request);

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
  static int lastPinStates[5] = { -1, -1, -1, -1, -1 };  // Only track our 5 relay pins
  static const int relayPins[5] = {18, 16, 5, 19, 17};  // Our actual relay pins
  
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
// 7. SETUP FUNCTION - GETTING EVERYTHING READY
// ============================================================================
//
// The setup function runs once when the ESP32 first turns on.
// It's like getting ready for school in the morning:
// - Wake up the different parts of the computer
// - Connect to the screen, sensors, and clock
// - Load saved settings from memory
// - Make a happy beep to show it's working
// - Get everything prepared for the main work
//
// This happens only once, then the loop function takes over forever.
//
// ============================================================================

  Serial.begin(115200);
  Serial.println(F("\n[SETUP] Starting ESP32 Fish Tank Automation System..."));
  Serial.printf(F("[SETUP] CPU Frequency: %dMHz\n"), getCpuFrequencyMhz());

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
    Serial.println(F("[NVS] Failed to initialize WiFi preferences."));
  }

  // Initialize I2C bus
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(100000);  // Standard mode (100kHz)
  Wire.setTimeout(5000);
#if DEBUG_MODE
  Serial.println(F("[I2C] I2C bus initialized."));
#endif

  int i = 0;
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println(F("[ERROR] RTC not found!"));
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
    Serial.println(F("[RTC] RTC is NOT running, setting time from build time."));
#endif
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set to compile time as fallback
  }
#if DEBUG_MODE
  Serial.println(F("[RTC] RTC initialized."));
#endif

  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
#if DEBUG_MODE
    Serial.println(F("[ERROR] OLED init failed"));
#endif
  } else {
#if DEBUG_MODE
    Serial.println(F("[OLED] Display initialized."));
#endif
    display.clearDisplay();
    display.setTextSize(2);  // Larger text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);  // Center-ish vertically for 64px display
    display.println("Bismillah");
    display.display();
    delay(2000);  // Show for 2 seconds
    display.clearDisplay();
    display.display();
  }

  // Initialize DS18B20 Temperature Sensor
  sensors.begin();
  sensors.setResolution(12);            // Set to 12-bit resolution for better accuracy
  sensors.setWaitForConversion(false);  // Use non-blocking mode

  // Initialize sensor state variables
  temperatureReadFailures = 0;
  temperatureRecoveryCount = 0;

  // Take an initial temperature reading
  sensors.requestTemperatures();
  delay(200);  // Wait for conversion
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
  // server.on("/refresh", HTTP_POST, handleForceRefresh);
  server.onNotFound(handleNotFound);

  // Start the server
  server.begin();
#if DEBUG_MODE
  Serial.println("[API] REST API server started.");
#endif
  Serial.println("[SETUP] System ready!");
}

/**
 * @brief Handles the "/restart" GET endpoint. Reboots the ESP32 after authenticating the API key.
 * Responds with JSON and then triggers ESP.restart().
 */
void handleRestart(AsyncWebServerRequest *request) {
  if (!authenticateRequest(request)) return;
  request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"System will reboot now\"}\n");
  delay(1000);  // Allow response to be sent
  ESP.restart();
}

// ============================================================================
// 8. MAIN LOOP - THE HEARTBEAT OF OUR SYSTEM
// ============================================================================
//
// The loop function runs forever while the ESP32 is turned on.
// It's like the main job that keeps happening over and over:
// - Check if WiFi is still connected (reconnect if needed)
// - Get the current time from the clock
// - Read the water temperature
// - Decide what devices should be on/off
// - Update the screen with current information
// - Handle any web requests from phones/computers
// - Keep everything running smoothly
//
// This loop runs thousands of times per second, keeping your fish tank happy!
//
// ============================================================================

  // Periodically sync RTC with NTP (every 3 hours, if WiFi available)
  if (wifiConnected && ((long)(millis() - lastNtpSyncMillis) >= NTP_SYNC_INTERVAL_MS)) {
    syncTimeNTP();
    lastNtpSyncMillis = millis();
  }

  // Get current time from RTC
  DateTime now = rtc.now();
  int currentMinutes = now.hour() * 60 + now.minute();  // Convert to minutes once

  // Read temperature more frequently if sensor error
  unsigned long tempReadInterval = tempSensorError ? TEMP_READ_ERROR_INTERVAL_MS : TEMP_READ_INTERVAL_MS;
  if ((long)(millis() - lastTempReadMillis) >= tempReadInterval) {
    sensors.requestTemperatures();
    delay(50);  // Give sensor time to complete conversion
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
      } else {
        temperatureRecoveryCount = 0;  // Reset recovery counter when sensor is stable
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
    if (lastTempSensorErrorBuzzMillis == 0 || (millis() - lastTempSensorErrorBuzzMillis) >= TEMP_SENSOR_ERROR_BUZZ_INTERVAL_MS) {
      buzz(3, 300);  // 3 quick buzzes
      lastTempSensorErrorBuzzMillis = millis();
    }
  }

  // Check emergency conditions - exclude zero/invalid values to prevent false alarms
  bool currentEmergencyState = false;
  EmergencyReason newEmergencyReason = NO_EMERGENCY;

  if (currentTemperatureC > 0.0) {
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
  if ((long)(millis() - lastOLEDUpdateMillis) >= OLED_UPDATE_INTERVAL_MS) {
    updateOLED(now);
    lastOLEDUpdateMillis = millis();
  }

  // Handle delayed schedule saving
  handleDelayedScheduleSave();
}

// ============================================================================
// 9. WIFI MANAGEMENT - STAYING CONNECTED TO THE INTERNET
// ============================================================================
//
// WiFi is how our fish tank controller talks to the internet and your devices.
// This section handles:
// - Connecting to your home WiFi network
// - Creating a backup hotspot if WiFi fails
// - Remembering WiFi passwords securely
// - Automatically reconnecting if connection drops
//
// Why WiFi matters:
// - Lets you control the tank from your phone
// - Gets accurate time from the internet
// - Sends alerts if something needs attention
// - Allows remote monitoring from anywhere
//
// ============================================================================
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
  WiFi.begin(ssid.c_str(), password.c_str());

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
    if (++dotCount % 3 == 0) {
      display.clearDisplay();
      display.setTextSize(1);  // Smallest font for max info
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println(F("Connecting to SSID:"));
      // Scroll SSID if too long
      int maxVisibleChars = 18;  // 18 chars at size 1 (128px/7px)
      int ssidLen = ssid.length();
      String ssidToShow;
      if (ssidLen > maxVisibleChars) {
        // Scroll the SSID horizontally
        if (millis() - lastScrollMillis > scrollDelay) {
          ssidScrollOffset = (ssidScrollOffset + 1) % (ssidLen + 2);  // +2 for smooth loop
          lastScrollMillis = millis();
        }
        if (ssidScrollOffset + maxVisibleChars <= ssidLen) {
          ssidToShow = ssid.substring(ssidScrollOffset, ssidScrollOffset + maxVisibleChars);
        } else {
          // Wrap around
          int firstPart = ssidLen - ssidScrollOffset;
          ssidToShow = ssid.substring(ssidScrollOffset);
          ssidToShow += "  ";
          ssidToShow += ssid.substring(0, maxVisibleChars - firstPart);
        }
      } else {
        ssidToShow = ssid;
      }
      display.setCursor(0, 10);
      display.println(ssidToShow);
      snprintf(oledBuffer, sizeof(oledBuffer), "Attempt %d/%d", wifiReconnectAttempts + 1, MAX_WIFI_RECONNECT_ATTEMPTS);
      display.setCursor(0, 20);
      display.println(oledBuffer);
      display.display();
    }
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

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("AP Mode Active:");
  display.setCursor(0, 10);
  display.println("SSID: MyTank");
  display.setCursor(0, 20);
  display.println("Pass: password@123");
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

  // Hang-on Filter: 6:30 AM - 8:30 AM (390-510), 8:30 PM - 10:30 PM (1230-1350)
  applianceSchedules["HangOnFilter"].push_back({ "on_interval", 390, 510 });
  applianceSchedules["HangOnFilter"].push_back({ "on_interval", 1230, 1350 });

  // Filter: 1:30 PM - 3:30 PM (810-930) - OFF during this time for maintenance
  applianceSchedules["Filter"].push_back({ "off_interval", 810, 930 });

  markSchedulesDirty();  // Schedule delayed save

#if DEBUG_MODE
  Serial.println(F("[Factory Reset] Default schedules loaded:"));
  Serial.println(F("  CO2: 8:30-13:30 (510-810), 15:30-20:30 (930-1230)"));
  Serial.println(F("  Light: 9:30-13:30 (570-810), 16:30-20:30 (990-1230)"));
  Serial.println("  Heater: 0:00-4:30 (0-270), 20:30-23:59 (1230-1439)");
  Serial.println("  HangOnFilter: 6:30-8:30 (390-510), 20:30-22:30 (1230-1350)");
  Serial.println("  Filter: OFF 13:30-15:30 (810-930), otherwise ON");
#endif
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

// ============================================================================
// 11. DEVICE CONTROL - DECIDING WHAT TURNS ON/OFF
// ============================================================================
//
// This is the smart brain that decides when each device should be on or off.
// It considers three things in order of importance:
//
// 1. EMERGENCY SAFETY (most important)
//    - If temperature is too high/low, turn off everything to protect fish
//
// 2. MANUAL CONTROL (second priority)
//    - If you manually turned something on/off from your phone, respect that
//
// 3. SCHEDULE CONTROL (normal operation)
//    - Follow the programmed schedule (like "lights on at 8 AM")
//    - Special temperature control for heater (keeps water perfect)
//
// For the heater, it also considers:
// - Turn on if water drops below 25°C
// - Turn off if water reaches 29°C
// - Make sure it runs at least 30 minutes when it turns on
//
// This logic runs every few seconds to keep everything just right!
//
// ============================================================================
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
  if (app.name == "Heater") {
    if (tempSensorError) {
      // Sensor error: allow manual overrides to take precedence (user is always priority)
      // Only fall back to scheduled state if no active override
      if (app.overrideEndTime == 0) {
        targetState = app.scheduledState;
        app.currentMode = SCHEDULED;
      } else {
        // Keep override state as set above
        app.currentMode = OVERRIDDEN;
      }
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
  }

  // Update appliance state only if changed (optimized)
  if (app.currentState != targetState) {
    app.currentState = targetState;
    setRelayState(app.pin, app.currentState);  // Use configurable relay control
    // Immediately update OLED to reflect the change
    DateTime now = rtc.now();
    updateOLED(now);
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

// ============================================================================
// 12. SCREEN DISPLAY - SHOWING INFORMATION ON THE OLED SCREEN
// ============================================================================
//
// The OLED screen is like a tiny TV that shows what's happening with your fish tank.
// It displays important information in real-time so you can see at a glance:
//
// Top line: Current time and date
// Next lines: Status of each device (ON/OFF)
// Bottom: Temperature and any alerts
//
// The screen updates every 2 seconds to show the latest information.
// It uses short names to fit everything on the small screen:
// - Filter → Filter
// - HangOnFilter → HOF (Hang-On Filter)
// - CO2 → CO2
// - Light → Light
// - Heater → Heater
//
// If there's an emergency, it shows warning messages.
// If WiFi is disconnected, it shows connection status.
//
// ============================================================================
void updateOLED(DateTime now) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Row 1: Current Time
  display.setCursor(0, 0);
  snprintf(oledBuffer, sizeof(oledBuffer), "%02d:%02d:%02d %02d/%02d",
           now.hour(), now.minute(), now.second(), now.day(), now.month());
  display.print(oledBuffer);

  // Row 2 onwards: Appliance States with compact formatting
  int y = 10;
  for (int i = 0; i < NUM_APPLIANCES; i++) {
    display.setCursor(0, y + (i * 8));
    // Use custom short names for better readability
    const char *shortName;
    if (appliances[i].name == "Filter") shortName = "Filter";
    else if (appliances[i].name == "HangOnFilter") shortName = "HOF";
    else if (appliances[i].name == "CO2") shortName = "CO2";
    else if (appliances[i].name == "Light") shortName = "Light";
    else if (appliances[i].name == "Heater") shortName = "Heater";
    else shortName = appliances[i].name.c_str();

    snprintf(oledBuffer, sizeof(oledBuffer), "%s: %s", shortName,
             (appliances[i].currentState == ON ? "ON" : "OFF"));
    display.print(oledBuffer);
  }

  // Temperature and System Status (bottom row)
  static unsigned long lastScrollMillis = 0;
  static int scrollOffset = 0;
  const char *errorMsg = "TEMP SENSOR NOT DETECTED - CHECK CONNECTION!  ";
  int maxVisibleChars = 21;  // 128px/6px per char (font size 1)
  int errorMsgLen = strlen(errorMsg);

  display.setCursor(0, y + (NUM_APPLIANCES * 8));
  if (tempSensorError) {
    // Running text (marquee) for error message
    if (millis() - lastScrollMillis > 200) {  // Scroll every 200ms
      scrollOffset = (scrollOffset + 1) % errorMsgLen;
      lastScrollMillis = millis();
    }
    char scrollBuf[32];
    for (int i = 0; i < maxVisibleChars; i++) {
      scrollBuf[i] = errorMsg[(scrollOffset + i) % errorMsgLen];
    }
    scrollBuf[maxVisibleChars] = '\0';
    display.print(scrollBuf);
  } else {
    snprintf(oledBuffer, sizeof(oledBuffer), "%.1fC", currentTemperatureC);
    display.print(oledBuffer);
    display.setCursor(40, y + (NUM_APPLIANCES * 8));
    if (emergencyShutdown) {
      // Display specific emergency reason
      switch (currentEmergencyReason) {
        case TEMP_TOO_HIGH:
          display.print(F("TEMP HIGH"));
          break;
        case TEMP_TOO_LOW:
          display.print(F("TEMP LOW"));
          break;
        case SENSOR_FAILURE:
          display.print(F("SENSOR"));
          break;
        default:
          display.print(F("EMRG!"));
          break;
      }
    } else if (WiFi.status() == WL_CONNECTED) {
      // Show more meaningful IP info - last two octets
      IPAddress ip = WiFi.localIP();
      snprintf(oledBuffer, sizeof(oledBuffer), "%d.%d", ip[2], ip[3]);
      display.print(oledBuffer);
    } else if (apModeActive) {
      display.print(F("AP"));
    } else {
      display.print(F("NoWiFi"));
    }
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
// 10. WEB API ENDPOINTS - LETTING PHONES/COMPUTERS TALK TO US
// ============================================================================
//
// The web API is how your phone or computer communicates with the fish tank controller.
// It's like a language that lets different devices understand each other.
//
// Available commands (endpoints):
// - /status: Check temperature, time, and what devices are on/off
// - /control: Turn devices on/off manually
// - /schedules: Set when devices should automatically turn on/off
// - /wifi: Configure WiFi settings
//
// How it works:
// 1. Your device sends a web request (like visiting a website)
// 2. Our ESP32 receives the request and understands what you want
// 3. ESP32 sends back information in JSON format (computer-readable text)
// 4. Your phone app shows the information in a nice interface
//
// Security: All requests need a special API key to prevent unauthorized access.
//
// ============================================================================
void handleRoot(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", "ESP32 Fish Tank Automation System API. Use /status, /control, /schedules, /wifi.");
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

  // Update OLED to ensure it matches the API response
  updateOLED(now);

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
    StaticJsonDocument<1024> doc;  // Use StaticJsonDocument for ArduinoJson v6
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
        request->send(404, FPSTR(STR_CONTENT_TYPE_JSON), F("{\"error\": \"Appliance not found\"}\n"));
        return;
      }
    }
    request->send(200, FPSTR(STR_CONTENT_TYPE_JSON), F("{\"status\": \"success\", \"message\": \"Appliance control updated\"}\n"));
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
    StaticJsonDocument<2048> doc;  // Use StaticJsonDocument for ArduinoJson v6
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

    request->send(200, "application/json", "{\"status\": \"success\", \"message\": \"Schedules updated and applied immediately\"}\n");
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
    StaticJsonDocument<512> doc;  // WiFi config - small size needed
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
