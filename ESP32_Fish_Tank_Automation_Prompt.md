# ESP32 Fish Tank Automation System - Code Generation Prompt

## System Overview
Generate a complete ESP32 fish tank automation system (Arduino sketch) that manages 5 aquarium appliances with intelligent scheduling, temperature control, WiFi connectivity, and REST API interface. This is a production-ready system with safety features and memory optimizations.

## Hardware Specifications

### Microcontroller
- **ESP32 Dev Module (38-pin)** with WiFi capability

### Components & Connections
- **DS1307 RTC Module**: SDA (GPIO 21), SCL (GPIO 22) - Real-time clock with NTP synchronization
- **DS18B20 Temperature Sensor**: Data (GPIO 14) with 4.7KΩ pull-up resistor to 3.3V
- **SSD1306 OLED Display (0.96", 128x64)**: SDA (GPIO 21), SCL (GPIO 22), I2C address 0x3C
- **8-Channel Relay Module (12V)**: Active LOW control, separate 12V supply for JD-VCC
- **Buzzer**: GPIO 13 (PWM capable) for audio feedback
- **Power**: ESP32 powered via USB/external 5V, relays via separate 12V supply

### Appliances & GPIO Mapping
1. **Filter** (Main filter pump): GPIO 16
2. **CO2** (CO2 injection system): GPIO 17  
3. **Light** (Aquarium lighting): GPIO 18
4. **Heater** (Water heater): GPIO 19
5. **HangOnFilter** (Hang-on filter): GPIO 20

## Core Functionality Requirements

### 1. WiFi & Network Management
- **Station Mode**: Connect to saved WiFi credentials stored in NVS
- **Access Point Mode**: Fallback "Fishtank_Setup" AP (password: "password123") if connection fails
- **Connection Timeout**: 30 seconds before switching to AP mode
- **WiFi Recovery**: Automatic reconnection attempts every 5 seconds
- **Audio Feedback**: Buzzer alerts for connection issues (3 buzzes, 1000ms interval)

### 2. Time Management System
- **Primary Clock**: DS1307 RTC module for accurate timekeeping
- **NTP Synchronization**: Automatic sync every 3 hours when WiFi connected (IST timezone +5.5 hours)
- **Fallback Time**: Use compile-time if RTC not running
- **Time Format**: 24-hour format, minute-based scheduling for efficiency

### 3. Appliance Control Logic
- **Schedule-Based Control**: Default ON/OFF intervals for each appliance
- **Manual Override**: Temporary control via REST API with optional timeout
- **Temperature Control**: Intelligent heater management (25°C-29°C range, 30-min minimum runtime)
- **Priority System**: Temperature control > Manual override > Schedule
- **Safety Features**: Emergency shutdown on extreme temperatures (>35°C or <10°C)

### 4. Default Schedules (Time in Minutes from Midnight)
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
- **Appliance Schedules**: JSON format storage with auto-save
- **Namespaces**: "wifi_creds" and "app_schedules"
- **Default Loading**: Auto-generate default schedules if NVS empty

## REST API Specifications

### Server Configuration
- **Port**: 80 (HTTP)
- **Authentication**: X-API-Key header with value "Automate@123"
- **Content-Type**: application/json
- **Body Size Limit**: 2048 bytes for POST requests

### API Endpoints

#### 1. GET `/status`
Returns system status including current time, temperature, and appliance states with control modes.

#### 2. POST `/control`
Control individual or multiple appliances with optional timeout.
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
- **Display Optimization**: Minimal text updates
- **Loop Efficiency**: Non-blocking operations with millis() timing

## Safety & Emergency Features

### Emergency Conditions
- **High Temperature**: > 35°C (immediate shutdown)
- **Low Temperature**: < 10°C (immediate shutdown)
- **Emergency Shutdown**: Turn OFF all appliances, sound alarm (5 buzzes @ 2kHz)

### Audio Feedback System
- **WiFi Issues**: 3 buzzes, 1000ms intervals
- **Emergency Alert**: 5 rapid buzzes @ 2kHz frequency
- **Confirmation**: Single buzz for successful operations
- **LEDC API**: Use ledcWriteTone() for PWM buzzer control

## Required Libraries
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
