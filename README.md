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
- Default schedules optimized for planted aquarium
- Heater: 25-29°C, 30-min minimum runtime
- Emergency shutdown: >32°C or <15°C

### Default Schedules (IST - India Standard Time)

**CO2 System:**
- 8:00 AM - 2:00 PM (6 hours - starts 1 hour before first light)
- 3:00 PM - 8:00 PM (5 hours - resumes with third light period)
- Total: 11 hours with 1-hour break (2:00 PM - 3:00 PM) for fish to relax during light-off period

**Aquarium Light:**
- 9:00 AM - 11:00 AM (Morning - 2 hours)
- 12:00 PM - 2:00 PM (Midday - 2 hours)
- 3:00 PM - 5:00 PM (Afternoon - 2 hours)
- 6:00 PM - 8:00 PM (Evening - 2 hours)
- Total: 8 hours with triple siesta breaks (maximum algae disruption + red plant vitality)

**Heater (Temperature Control):**
- 12:00 AM - 4:30 AM (Night heating)
- 8:30 PM - 11:59 PM (Evening heating)
- Auto ON when temp < 25°C, OFF when temp ≥ 29°C

**Hang-on Filter:**
- 9:00 AM - 10:30 AM (Morning maintenance)
- 7:30 PM - 8:30 PM (Evening maintenance)

**Wave Maker:**
- 10:30 AM - 12:30 PM (Midday circulation)
- 3:30 PM - 7:30 PM (Afternoon circulation)

**Main Filter:**
- ON 24/7 (continuous filtration)
- OFF 1:30 PM - 3:30 PM (maintenance window)

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


