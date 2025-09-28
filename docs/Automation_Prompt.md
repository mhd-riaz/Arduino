# ESP32 Fish Tank Automation Commercial Product - Professional Code Generation Prompt

## Commercial Product Overview
Generate a complete ESP32 fish tank automation system (Arduino sketch) for commercial end-user product sales. This professional control PCB manages 5 customer-provided aquarium appliances with intelligent scheduling, temperature control, WiFi connectivity, and comprehensive REST API interface. Designed for professional installation with external power supplies and customer-provided components.

## Commercial Hardware Specifications

### Professional Microcontroller
- **ESP32 Dev Module (38-pin)** with WiFi capability - Built into PCB

### Customer-Provided External Components & Professional Connections
- **DS3231 RTC Module**: Customer connects to PCB J3 terminal (SDA GPIO 21, SCL GPIO 22)
- **DS18B20 Temperature Sensor**: Customer installs and connects to PCB J4 terminal (Data GPIO 14) with built-in 4.7KΩ pull-up
- **SSD1306 OLED Display (0.96", 128x64)**: Customer connects to PCB J3 terminal (shared I2C, address 0x3C)
- **8-Channel Relay Module (12V)**: Customer connects to PCB J5 terminal with external 12V SMPS
- **Buzzer**: Customer connects to PCB J8 terminal (PWM GPIO 13) for audio feedback
- **Power Architecture**: Customer provides ERD Charger 5V/2A via J1 and SMPS 12V/2A via J2

### Professional Appliance Control & Customer Equipment
1. **Filter** (Customer 20W main filter pump): GPIO 18 via J5 Terminal
2. **CO2** (Customer 15W CO2 injection system): GPIO 16 via J5 Terminal
3. **Light** (Customer 40W LED aquarium lighting): GPIO 5 via J5 Terminal
4. **Heater** (Customer 200W water heater): GPIO 19 via J5 Terminal
5. **HangOnFilter** (Customer 20W hang-on filter): GPIO 17 via J5 Terminal

## Commercial Product Core Functionality Requirements

### 1. Professional WiFi & Network Management
- **Station Mode**: Connect to customer WiFi credentials stored in NVS
- **Access Point Mode**: Fallback "FishTank_Commercial" AP (password: "Commercial123") for customer setup
- **Professional Connection Timeout**: 30 seconds before switching to AP mode
- **WiFi Recovery**: Automatic reconnection attempts every 5 seconds for commercial reliability
- **Customer Audio Feedback**: Buzzer alerts for connection issues (3 buzzes, 1000ms interval)

### 2. Commercial Time Management System
- **Primary Clock**: Customer DS3231 RTC module for precise commercial timekeeping
- **NTP Synchronization**: Automatic sync every 3 hours when WiFi connected (IST timezone +5.5 hours)
- **Fallback Time**: Use compile-time if customer RTC not running
- **Time Format**: 24-hour format, minute-based scheduling for professional efficiency

### 3. Commercial Appliance Control Logic
- **Schedule-Based Control**: Professional ON/OFF intervals for customer appliances
- **Manual Override**: Temporary control via REST API with optional timeout for maintenance
- **Professional Temperature Control**: Intelligent 200W heater management (25°C-29°C range, 30-min minimum runtime)
- **Priority System**: Temperature control > Manual override > Schedule for optimal commercial operation
- **Commercial Safety Features**: Emergency shutdown on extreme temperatures (>32°C or <20°C) to protect aquatic life

### 4. Commercial Default Schedules (Time in Minutes from Midnight)
```cpp
// CO2: 8:30 AM - 1:30 PM (510-810), 3:30 PM - 8:30 PM (930-1230)
CO2: on_interval 510-810, on_interval 930-1230

// Light: 9:30 AM - 1:30 PM (570-810), 4:30 PM - 8:30 PM (990-1230)  
Light: on_interval 570-810, on_interval 990-1230

// Heater: 12:00 AM - 4:30 AM (0-270), 8:30 PM - 11:59 PM (1230-1439)
Heater: on_interval 0-270, on_interval 1230-1439

// HangOnFilter: 6:30 AM - 8:30 AM (390-510), 8:30 PM - 10:30 PM (1230-1350)
HangOnFilter: on_interval 390-510, on_interval 1230-1350

// Filter: Always ON except 1:30 PM - 3:30 PM (810-930) for maintenance
Filter: off_interval 810-930
```

### 5. Temperature-Controlled Heater Logic
- **Turn ON**: When temperature < 25.0°C
- **Turn OFF**: When temperature ≥ 29.0°C (with minimum 30-minute runtime)
- **Minimum Runtime**: 30 minutes continuous operation once activated
- **Priority**: Temperature control overrides schedule and manual overrides
- **Reading Frequency**: Every 5 seconds

### 6. Data Storage (NVS)
- **WiFi Credentials**: SSID and password persistence
- **Professional Data Storage**: JSON format with auto-save for commercial reliability
- **Secure Namespaces**: "wifi_creds" and "app_schedules" for customer data protection
- **Default Configuration**: Auto-generate professional default schedules if NVS empty

## Commercial REST API Specifications

### Professional Server Configuration
- **Port**: 80 (HTTP) for customer web interface access
- **Authentication**: X-API-Key header with value "Automate@123" for secure commercial operation
- **Content-Type**: application/json for professional API standards
- **Body Size Limit**: 2048 bytes for POST requests to prevent memory issues

### Commercial API Endpoints

#### 1. GET `/status`
Returns comprehensive system status including current time, temperature, and appliance states with control modes for professional monitoring.

#### 2. POST `/control`
Professional control interface for individual or multiple customer appliances with optional timeout for maintenance operations.
```json
// Single appliance
{
  "appliance": "CO2",
  "action": "ON",
  "timeout_minutes": 60
}

// Multiple appliances
{
  "appliances": [
    {"name": "Light", "action": "OFF", "timeout_minutes": 30},
    {"name": "Filter", "action": "ON"}
  ]
}
```

#### 3. GET `/schedules`
Returns all stored appliance schedules.

#### 4. POST `/schedules`
Update all appliance schedules.
```json
{
  "schedules": {
    "CO2": [
      {"type": "on_interval", "start_min": 510, "end_min": 810}
    ]
  }
}
```

#### 5. POST `/wifi`
Update WiFi credentials (triggers system restart).
```json
{
  "ssid": "NewNetworkName",
  "password": "NewPassword"
}
```

#### 6. POST `/emergency/reset`
Reset emergency shutdown state (only if conditions are safe).

### Error Handling
- **401**: Missing/invalid API key
- **400**: Invalid JSON or missing required fields
- **404**: Appliance not found
- **413**: Request body too large

## Display System (OLED)

### Display Content
- **Line 1**: Current time (HH:MM:SS DD/MM)
- **Lines 2-6**: Appliance states (Name: ON/OFF)
- **Line 7**: Temperature (XX.XºC) + Status (WiFi: OK/AP Mode/No WiFi/EMERGENCY!)
- **Update Frequency**: Every 2 seconds (memory optimized)

### Status Indicators
- **WiFi: OK**: Connected to network
- **AP Mode**: Access point active
- **No WiFi**: Disconnected
- **EMERGENCY!**: System in emergency shutdown

## Memory & Performance Optimizations

### Code Optimizations
1. **Minute-Based Scheduling**: Convert all times to minutes (0-1439) for efficient comparison
2. **Single POST Buffer**: Reusable String buffer for all POST requests  
3. **Reduced OLED Updates**: 2-second intervals instead of continuous updates
4. **ArduinoJson v6**: Modern JsonDocument instead of StaticJsonDocument
5. **Selective NTP Sync**: Only sync when WiFi available, 3-hour intervals

### Memory Management
- **Buffer Limits**: 2KB POST body, 4KB JSON documents
- **NVS Namespaces**: Separate WiFi and schedule storage
- **Display Optimization**: Minimal text updates for commercial efficiency
- **Loop Efficiency**: Non-blocking operations with millis() timing for professional performance

## Commercial Safety & Emergency Features

### Professional Emergency Conditions
- **High Temperature**: > 32°C (immediate shutdown) - Commercial aquatic life protection
- **Low Temperature**: < 20°C (immediate shutdown) - Professional temperature range
- **Emergency Shutdown**: Turn OFF all customer appliances, sound alarm (5 buzzes @ 2kHz)

### Commercial Audio Feedback System
- **WiFi Issues**: 3 buzzes, 1000ms intervals for customer notification
- **Emergency Alert**: 5 rapid buzzes @ 2kHz frequency for critical situations
- **Confirmation**: Single buzz for successful operations and customer feedback
- **LEDC API**: Use ledcWriteTone() for PWM buzzer control with ESP32 Core 3.0+ compatibility

## Required Professional Libraries
```cpp
#include <WiFi.h>                    // ESP32 built-in
#include <ESPAsyncWebServer.h>       // https://github.com/me-no-dev/ESPAsyncWebServer
#include <AsyncTCP.h>                // Dependency for ESPAsyncWebServer
#include <Preferences.h>             // ESP32 built-in NVS
#include <Wire.h>                    // Arduino built-in I2C
#include <RTClib.h>                  // https://github.com/adafruit/RTClib
#include <OneWire.h>                 // https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>       // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <Adafruit_GFX.h>           // https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h>       // https://github.com/adafruit/Adafruit_SSD1306
#include <ArduinoJson.h>            // https://github.com/bblanchon/ArduinoJson (v6)
#include <NTPClient.h>              // https://github.com/arduino-libraries/NTPClient
#include <WiFiUdp.h>                // ESP32 built-in
#include <map>                      // C++ STL for schedule storage
#include <driver/ledc.h>            // ESP32 LEDC PWM for buzzer
```

## Code Structure Requirements

### Main Components
1. **Pin Definitions**: All GPIO mappings and constants
2. **Global Variables**: System state, schedules, network objects
3. **Setup Function**: Hardware initialization, WiFi connection, API server start
4. **Main Loop**: Non-blocking time management, appliance control, display updates
5. **Helper Functions**: WiFi management, NTP sync, schedule operations
6. **API Handlers**: All REST endpoints with proper JSON handling
7. **Safety Functions**: Emergency shutdown and recovery

### Key Implementation Details
- **Active LOW Relays**: LOW = ON, HIGH = OFF
- **Non-Blocking Operations**: Use millis() for all timing
- **Error Handling**: Comprehensive error responses and logging
- **Memory Safety**: Buffer size checks and memory reservation
- **Production Ready**: Robust error handling and recovery mechanisms

## Version Information
- **Version**: 1.5 - LEDC API Fix
- **Author**: mhd-riaz  
- **Target**: ESP32 Dev Module
- **Date**: June 2, 2025
- **Status**: Production-ready with optimizations

## Testing & Validation
- **WiFi Connection**: Test both STA and AP modes
- **API Endpoints**: Verify all REST operations with proper authentication
- **Temperature Control**: Test heater logic with actual temperature readings
- **Schedule System**: Validate all appliance schedules at different times
- **Emergency System**: Test safety shutdown and recovery
- **Memory Usage**: Ensure stable operation with minimal memory footprint

Generate a complete, production-ready Arduino sketch (.ino file) that implements all these specifications exactly as described. The code should be well-documented, memory-optimized, and include all necessary error handling and safety features.
