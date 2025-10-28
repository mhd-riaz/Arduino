# Arduino Commercial Projects

## ESP32 Fish Tank Automation Commercial Product ✅

### **Product Overview**
- Professional control PCB designed for end-user sales with external power supplies
- External customer components: SMPS 12V/2A, ERD Charger 5V/2A, 6A MCB protection  
- Professional installation by qualified electrician required
- Customer-provided appliances: 200W heater, 40W LED, 20W pumps, 20W filters

### **Current Version**: 3.0.1
### **Release Date**: October 28, 2025
### **Status**: Commercial Product Ready for Manufacturing ✅

### **Documentation & Design Files**
- [📋 **REST API Documentation**](Documentation.md) - Comprehensive API reference
- [⚡ **KiCad Schematic Design**](ESP32_Fish_Tank_Schematic.md) - Professional PCB design files
- [🔌 **Connection Diagrams**](Connection_Diagram.md) - Professional installation guide  
- [📊 **KiCad Symbol Library**](KiCad_Symbol_Library.md) - Custom component symbols
- [📝 **Changelog**](CHANGELOG.md) - Version history and feature tracking

---

## 📚 System Analysis & Architecture

### **Application & Purpose**

This is a **commercial-grade aquarium automation controller** (Version 3.0) designed for professional deployment with plug-and-play configuration. The system intelligently automates and monitors aquarium operations through:

- **Intelligent scheduling** - Custom minute-based schedules for each appliance
- **Temperature control** - Automatic heater management with 25-29°C target range
- **Remote management** - REST API with JSON interface for system control
- **Real-time monitoring** - OLED display for status visualization
- **Safety features** - Emergency shutdown system for extreme conditions
- **Persistent storage** - NVS (Non-Volatile Storage) for settings persistence

---

## 🎯 Controlled Appliances (6 Devices)

| Device | Power | Function | Default Behavior |
|--------|-------|----------|------------------|
| **Filter** | 20W | Main filtration pump | ON by default, OFF during 13:30-15:30 |
| **CO2** | 15W | CO2 injection system | ON 08:30-13:30, 15:30-20:30 |
| **Light** | 40W | LED lighting | ON 09:30-13:30, 16:30-20:30 |
| **Heater** | 200W | Water temperature control | Temp-controlled 25-29°C |
| **HangOnFilter** | 20W | Secondary filtration | ON 09:00-10:30, 19:30-20:30 |
| **WaveMaker** | 10W | Water circulation | ON 10:30-12:30, 15:30-19:30 |

---

## ⚙️ System Architecture

### **Hardware Architecture**

```
AC Mains (240V)
    ↓
[6A MCB + Surge Protection] (Customer Provided)
    ↓
    ├─→ SMPS 12V/2A (Customer Provided) → Relay Power Supply
    │
    └─→ ERD Charger 5V/2A (Customer Provided) → ESP32 System
            ↓
    ┌───────────────────────────────────────┐
    │    ESP32 Control PCB (Product)        │
    │  • DS3231 RTC                         │
    │  • DS18B20 Temp Sensor                │
    │  • 0.96" OLED Display                 │
    │  • 8-Channel Relay Module             │
    │  • Professional Protection Circuits   │
    └───────────────────────────────────────┘
            ↓
    [12V Optocoupler Relay Module] → Appliances
```

### **Pin Configuration**

| Component | Pin(s) | Function |
|-----------|--------|----------|
| **RTC (DS3231)** | GPIO 21 (SDA), 22 (SCL) | I2C Bus - Time management |
| **OLED Display** | GPIO 21 (SDA), 22 (SCL) | I2C Bus - Status display |
| **Temperature Sensor (DS18B20)** | GPIO 14 | OneWire - Water temp monitoring |
| **Relay Controls** | GPIO 17, 19, 5, 16, 18, 32 | Active LOW - Appliance control |
| **Buzzer** | GPIO 13 | PWM - Audio feedback |

### **Communication Protocols**

- **I2C**: RTC + OLED (shared bus, 100kHz)
- **OneWire**: Temperature sensor (GPIO 14, requires 4.7KΩ pull-up)
- **WiFi**: ESP32 built-in (2.4GHz, with automatic AP fallback)
- **REST API**: HTTP/JSON on port 80

---

## 🔌 Professional Protection Components

### **Customer-Provided (AC Side)**
- **6A MCB** - Main circuit breaker for AC protection
- **275V MOV/SPD** - Surge protection device
- **30mA ELCB** - Earth leakage protection

### **Built into Control PCB (DC Side)**
- **3A Slow-blow Fuse (T3AL250V)** - 12V rail protection
- **2.5A Slow-blow Fuse (T2.5AL250V)** - 5V rail protection
- **1N4744A TVS Diodes** - 15V Zener surge protection
- **1N5819 Schottky Diodes** - Reverse polarity protection
- **470µF Capacitors** - Power filtering
- **Ferrite Beads** - EMI suppression (220Ω@100MHz)
- **330Ω Resistors** - GPIO current limiting
- **4.7kΩ Resistors** - I2C and OneWire pull-ups

---

## 💾 Time Management System

### **Minute-Based Schedule Format (0-1439 minutes per day)**

The system uses an efficient minute-based format where:
- **0 minutes** = 00:00 (midnight)
- **570 minutes** = 09:30
- **1230 minutes** = 20:30
- **1439 minutes** = 23:59

**Conversion Formula:**
```
Total Minutes = (Hour × 60) + Minute
```

### **Default Schedules**

```
CO2:          08:30-13:30 (510-810),    15:30-20:30 (930-1230)
Light:        09:30-13:30 (570-810),    16:30-20:30 (990-1230)
Heater:       00:00-04:30 (0-270),      20:30-23:59 (1230-1439)
HangOnFilter: 09:00-10:30 (540-630),    19:30-20:30 (930-1230)
WaveMaker:    10:30-12:30 (630-750),    15:30-19:30 (930-1170)
Filter:       OFF 13:30-15:30 (810-930), otherwise ON
```

### **NTP Synchronization**
- **Primary**: NTP (pool.ntp.org) every 3 hours when WiFi available
- **Fallback**: RTC (DS3231) maintains time during WiFi outages
- **Timezone**: IST (5.5 hour offset from UTC)

---

## 🎮 Control Priority System (Three-Tier)

The system uses an intelligent three-tier control hierarchy:

### **1. Temperature Control (HIGHEST PRIORITY)**
```
IF Heater AND Temperature < 25°C
  → Force Heater ON (even if schedule says OFF)
  → Maintain minimum 30-minute runtime
  → Release when temp ≥ 29°C

IF Temperature > 32°C or < 15°C
  → EMERGENCY SHUTDOWN all appliances
  → Sound alarm (5 buzzes at 2kHz)
  → Auto-recover after 30 minutes or when safe
```

### **2. Manual Overrides (MEDIUM PRIORITY)**
```
IF User Override Active
  → Apply override state (ON/OFF)
  → Respect timeout (permanent or temporary)
  → Prevents scheduled state from executing
```

### **3. Scheduled Control (LOWEST PRIORITY)**
```
IF No Override AND Not Temp-Controlled
  → Check schedule intervals
  → Apply scheduled ON/OFF state
  → Default: Filter ON, Others OFF
```

### **Control Mode Tracking**

Each appliance tracks its current control mode:
- **SCHEDULED** - Following programmed schedule
- **OVERRIDDEN** - Manual override active
- **TEMP_CONTROLLED** - Temperature-based (Heater only)

---

## 🌡️ Intelligent Heater Control Logic

### **Temperature Thresholds**
```
Target Zone: 25°C - 29°C

IF Temperature < 25°C
  → Activate heater (set heater_forced_on = true)
  → Start minimum runtime timer (30 minutes)

IF Temperature ≥ 29°C AND runtime ≥ 30 minutes
  → Deactivate heater
  → Reset forced state flag

IF 25°C ≤ Temperature < 29°C
  → Maintain current heater state
  → Don't force changes while in target zone
```

### **Emergency Temperature Limits**
```
IF Temperature > 32°C
  → EMERGENCY: Shutdown (Possible heater failure)

IF Temperature < 15°C
  → EMERGENCY: Shutdown (Possible sensor error or extreme cold)
```

### **Sensor Error Recovery**
```
Failure Threshold: 5 consecutive read failures
Recovery Requirement: 3 successful reads after failure
Recovery Count Tracks: Progress toward clearing emergency state
```

---

## 📡 REST API System

### **Authentication**
- **Method**: Header-based API key
- **Header**: `X-API-Key: Automate@123`
- **Applied to**: All endpoints except root

### **API Endpoints (9 Total)**

#### **1. GET /status**
Returns complete system status including appliance states, temperature, and emergency status.

```json
{
  "timestamp": "2025-10-28T14:30:45",
  "temperature_celsius": 26.5,
  "appliances": {
    "Filter": {"state": "ON", "mode": "SCHEDULED"},
    "CO2": {"state": "ON", "mode": "SCHEDULED"},
    "Light": {"state": "OFF", "mode": "SCHEDULED"},
    "Heater": {"state": "ON", "mode": "TEMP_CONTROLLED"},
    "HangOnFilter": {"state": "OFF", "mode": "SCHEDULED"},
    "WaveMaker": {"state": "OFF", "mode": "SCHEDULED"}
  },
  "emergency": false,
  "emergency_reason": "NONE",
  "sensor_error": false,
  "rtc_initialized": true,
  "oled_initialized": true
}
```

#### **2. POST /control**
Override appliance states with optional timeout.

```json
{
  "appliances": [
    {"name": "Light", "action": "ON", "timeout_minutes": 30},
    {"name": "Heater", "action": "OFF"},
    {"name": "Filter", "action": "ON", "timeout_minutes": 120}
  ]
}
```

#### **3. GET /schedules**
Retrieve all stored schedules in minute-based format.

```json
{
  "schedules": {
    "Filter": [
      {"type": "off_interval", "start_min": 810, "end_min": 930}
    ],
    "CO2": [
      {"type": "on_interval", "start_min": 510, "end_min": 810},
      {"type": "on_interval", "start_min": 930, "end_min": 1230}
    ]
  }
}
```

#### **4. POST /schedules**
Update all appliance schedules at once.

```json
{
  "schedules": {
    "Light": [
      {"type": "on_interval", "start_min": 570, "end_min": 810},
      {"type": "on_interval", "start_min": 990, "end_min": 1230}
    ],
    "CO2": [
      {"type": "on_interval", "start_min": 510, "end_min": 1080}
    ]
  },
  "clear_overrides": true
}
```

#### **5. POST /schedule**
Update schedule for a single appliance.

```json
{
  "appliance": "Light",
  "schedule": [
    {"type": "on_interval", "start_min": 420, "end_min": 1080}
  ],
  "clear_override": true
}
```

#### **6. POST /wifi**
Update WiFi credentials (triggers system reboot).

```json
{
  "ssid": "MyHomeNetwork",
  "password": "MyWifiPassword"
}
```

#### **7. POST /emergency/reset**
Reset emergency shutdown when conditions are safe.

#### **8. POST /reset**
Reset appliance overrides or perform factory reset.

```json
{
  "appliances": ["Light", "CO2"]
}
```

OR factory reset:
```json
{
  "reset_all": true
}
```

#### **9. GET /restart**
Reboot the ESP32 system.

---

## 💻 Code Style & Design Patterns

### **1. Memory Optimization**

**PROGMEM Flash Storage**
```cpp
const char PROGMEM STR_OK[] = "OK";           // Saves RAM
const char PROGMEM STR_ERROR[] = "Error";     // No RAM allocation
```

**Efficient JSON Handling**
- `StaticJsonDocument` for predictable memory usage
- Pre-allocated buffers (`json.reserve()`)
- Single reusable POST body buffer across all endpoints

**OLED Update Frequency**
- **Normal**: 2-second refresh intervals
- **Memory**: ~30mA baseline when OLED disabled

### **2. Safety & Reliability**

**Race Condition Prevention**
```cpp
bool heaterControlLock = false;  // Simple lock mechanism
```

**Sensor Error Handling**
- Requires 5 consecutive failures to trigger alert
- Requires 3 successful readings to clear alert
- Prevents false alarms from transient errors

**Non-Blocking Architecture**
- All timing uses `millis()` instead of `delay()`
- Allows concurrent tasks (WiFi, sensors, API)
- Graceful degradation if components fail

### **3. Professional Structure**

**Clear Code Organization** (10 Sections)
1. Library Includes & Compatibility
2. Pin Definitions
3. Global Constants & Variables
4. Function Prototypes
5. Helper Functions (Relay Control)
6. Setup & Loop
7. Core Logic (WiFi, Scheduling, Relays)
8. REST API Handlers
9. Emergency & Safety Functions
10. Header Comments with full specifications

**Extensive Documentation**
- Commercial product specifications
- Hardware protection details
- Indian electrical standards compliance
- Installation requirements

### **4. Time Management Efficiency**

**Minute-Based Format Advantages**
```cpp
// Efficient: only 2 int values per interval
struct ScheduleEntry {
  String type;
  int start_min;    // 0-1439 (not hours + minutes)
  int end_min;      // 0-1439
};
```

**Overnight Interval Handling**
```cpp
bool isTimeInInterval(int currentMinutes, int startMin, int endMin) {
  if (startMin <= endMin) {
    return (currentMinutes >= startMin && currentMinutes < endMin);
  } else {
    // 22:00-06:00 correctly handled
    return (currentMinutes >= startMin || currentMinutes < endMin);
  }
}
```

### **5. State Management**

**Comprehensive Appliance Tracking**
```cpp
struct Appliance {
  String name;                    // Device identifier
  int pin;                        // GPIO pin
  ApplianceState currentState;    // Physical state (ON/OFF)
  ApplianceState scheduledState;  // Schedule-based state
  ApplianceState overrideState;   // Manual override state
  unsigned long overrideEndTime;  // When override expires
  ApplianceMode currentMode;      // SCHEDULED/OVERRIDDEN/TEMP_CONTROLLED
};
```

**State Transition Logic**
- Clean separation between state sources
- Single decision point in `applyApplianceLogic()`
- Prevents conflicting control signals

### **6. REST API Design Patterns**

**Consistent JSON Format with Arrays**
```json
{
  "appliances": [
    {"name": "...", "action": "..."},
    {"name": "...", "action": "..."}
  ]
}
```

**Comprehensive Error Handling**
- 401 - Missing/invalid API key
- 400 - Invalid JSON or missing parameters
- 404 - Appliance/resource not found
- 413 - Request body too large
- 500 - Internal server error

**CORS Support**
```cpp
const char PROGMEM STR_ACCESS_CONTROL[] = "Access-Control-Allow-Origin";
const char PROGMEM STR_WILDCARD[] = "*";
```

### **7. Emergency System**

**Three-State Emergency Tracking**
```cpp
enum EmergencyReason {
  NO_EMERGENCY = 0,
  TEMP_TOO_HIGH,
  TEMP_TOO_LOW,
  SENSOR_FAILURE
};
```

**Automatic Recovery Logic**
- Timeout-based reset (30 minutes)
- Condition-based reset (temperature normalization)
- Safe state validation before recovery

### **8. Error Recovery Patterns**

**Sensor Read Failure Tracking**
```cpp
int temperatureReadFailures = 0;           // Current failures
int temperatureRecoveryCount = 0;          // Recovery progress
const int MAX_TEMP_FAILURES = 5;           // Threshold
const int MIN_RECOVERY_READINGS = 3;       // Recovery requirement
```

**Graceful Degradation**
- System runs with OLED disabled
- Heater reverts to scheduled state if sensor fails
- API continues operating through emergencies

---

## 🔧 Relay Control Configuration

### **Relay Type Selection**
```cpp
#define RELAY_ACTIVE_LOW false  // Professional optocoupler modules

// Helper functions abstract relay logic
int getRelayOnState()  { return RELAY_ACTIVE_LOW ? LOW : HIGH; }
int getRelayOffState() { return RELAY_ACTIVE_LOW ? HIGH : LOW; }
```

### **Redundancy Optimization**
```cpp
void setRelayState(int pin, ApplianceState state) {
  // Caches last state to avoid redundant GPIO writes
  // Only writes if state actually changed
}
```

---

## 📊 System Monitoring

### **OLED Display Information**
```
Line 1: HH:MM:SS DD/MM (Time and date)
Line 2-7: Device states with short names
         Filter: ON, CO2: ON, Light: OFF,
         Heater: ON, HOF: OFF, Wave: OFF
Line 8: Temperature OR WiFi info
        26.5°C OR 192.168 OR "AP" OR "NoWiFi"
```

### **Serial Debug Output** (When DEBUG_MODE = 1)
- System startup sequence
- WiFi connection status
- RTC synchronization events
- Appliance state changes (temperature-controlled)
- Temperature readings and sensor errors
- 30-second interval system health report

### **Buzzer Alerts**
- **1 beep**: Startup confirmation
- **3 beeps (1s apart)**: WiFi connection failed
- **3 quick beeps (300ms apart)**: Temperature sensor error (hourly)
- **5 buzzes (2kHz)**: Emergency shutdown activated

---

## 🏭 Manufacturing & Deployment

### **For End-Users (Installation)**
1. Customer purchases: SMPS 12V/2A, ERD Charger 5V/2A
2. Qualified electrician installs: 6A MCB, MOV, ELCB
3. Connect power supplies to control PCB
4. Configure WiFi via AP mode ("Fishtank_Setup")
5. Set custom schedules via REST API
6. Place OLED display on tank for monitoring

### **Firmware Updates**
- Use Arduino IDE with ESP32 board support
- Required libraries: WiFi, ESPAsyncWebServer, AsyncTCP, RTClib, Adafruit_SSD1306, ArduinoJson, NTPClient
- Compile time: ~60 seconds
- Upload time: ~5-10 seconds

### **Compliance & Safety**
- Follows Indian electrical standards (240V AC, 50Hz)
- Includes circuit protection at all power rails
- EMI suppression for reliable WiFi operation
- Thermal management through efficient scheduling
- Water safety through opto-isolated relays

---

## 📋 Key Technical Specifications

| Specification | Value |
|---------------|-------|
| **Microcontroller** | ESP32 (38-pin) |
| **Operating Voltage** | 5V (from ERD Charger) |
| **Relay Supply** | 12V (from SMPS) |
| **WiFi** | 2.4GHz, 802.11 b/g/n |
| **API Port** | 80 (HTTP) |
| **Max Appliances** | 6 devices |
| **Temperature Range** | -20°C to +85°C (sensor) |
| **RTC Accuracy** | ±2 ppm (DS3231) |
| **OLED Display** | 0.96" 128x64 pixels |
| **Response Time** | <200ms for relay control |
| **Memory** | 520KB RAM, 4MB Flash |
| **NVS Storage** | Persistent schedules & WiFi creds |
| **Update Frequency** | OLED: 2s, Sensors: 15s |
| **Heater Min Runtime** | 30 minutes continuous |
| **Emergency Timeout** | 30 minutes auto-reset |

---

## 🚀 Getting Started

### **Quick Setup**
1. Upload firmware to ESP32 using Arduino IDE
2. Power on system
3. Connect to "Fishtank_Setup" WiFi (password: `password@123`)
4. POST WiFi credentials to `/wifi` endpoint
5. System reboots and connects to your network
6. Access API at `http://<ESP32_IP>/`

### **First API Call**
```bash
curl -X GET http://192.168.1.100/status \
  -H "X-API-Key: Automate@123"
```

### **Customizing Schedules**
```bash
curl -X POST http://192.168.1.100/schedules \
  -H "X-API-Key: Automate@123" \
  -H "Content-Type: application/json" \
  -d '{
    "schedules": {
      "Light": [
        {"type": "on_interval", "start_min": 420, "end_min": 1080}
      ]
    }
  }'
```

---

## 📝 Version History

- **v3.0.1** (Oct 28, 2025) - Added OLED display rotation support (90° clockwise)
- **v3.0.0** (Oct 21, 2025) - Initial commercial product release with 6-device support
- **v2.x** - Previous versions with 5-device support
- **v1.x** - Initial prototype

**See [CHANGELOG.md](CHANGELOG.md) for complete version history and roadmap.**

---

**Last Updated**: October 28, 2025
**Status**: Production Ready ✅