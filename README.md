# ESP32 Fish Tank Automation System

## Version: 3.2.0
## Date: November 2, 2025

---

## Overview
Commercial fish tank automation system for end-user product deployment. Controls 6 appliances (Filter, CO2, Light, Heater, HangOnFilter, WaveMaker) with intelligent scheduling, temperature control, REST API, and real-time monitoring.

---

## Features
- Professional control PCB with modular external power supplies
- 6 appliances via relay interface
- Minute-based custom schedules
- DS3231 RTC + NTP sync
- DS18B20 temperature sensor (25-29°C heater control)
- OLED display (rotatable)
- REST API (9 endpoints)
- WiFi with AP fallback
- Emergency safety system
- Persistent NVS storage
- Buzzer alerts

---

## Hardware
- ESP32 Dev Module (38-pin)
- DS3231 RTC
- DS18B20 temp sensor
- 0.96" OLED (SSD1306)
- 8-channel 12V optocoupler relay
- Buzzer
- SMPS 12V/2A, ERD Charger 5V/2A (customer)
- 6A MCB (customer)

---

## Pinout
| Device         | GPIO | Function           |
|---------------|------|--------------------|
| Filter        | 16   | Main filtration    |
| CO2           | 17   | CO2 injection      |
| Light         | 18   | LED lighting       |
| Heater        | 19   | Water heater       |
| HangOnFilter  | 23   | Secondary filter   |
| WaveMaker     | 32   | Water movement     |
| Temp Sensor   | 33   | DS18B20 OneWire    |
| OLED/RTC      | 21/22| I2C SDA/SCL        |
| Buzzer        | 25   | PWM audio          |

---

## REST API Endpoints
- `/status` (GET): System status
- `/control` (POST): Override appliances
- `/schedules` (GET/POST): Manage schedules
- `/schedule` (POST): Update single appliance
- `/wifi` (POST): WiFi config
- `/emergency/reset` (POST): Emergency reset
- `/reset` (POST): Reset overrides/factory reset
- `/restart` (GET): Reboot system

API Key: `X-API-Key: Automate@123`

---

## Scheduling
- Minute-based intervals (0-1439)
- Default schedules for each appliance
- Heater: 25-29°C, 30-min minimum runtime
- Emergency shutdown: >32°C or <15°C

---

## Usage
1. Upload firmware to ESP32
2. Power on system
3. Connect to AP ("Fishtank_Setup") for WiFi config
4. Use REST API for control/scheduling
5. Monitor status on OLED

---

## Safety & Compliance
- Indian electrical standards (240V AC)
- Fuses, TVS diodes, EMI suppression
- Professional installation recommended

---