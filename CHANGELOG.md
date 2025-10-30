# Changelog - ESP32 Fish Tank Automation System

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## Semantic Versioning Guide

**Version Format: X.Y.Z**

- **X (Major)** - Breaking changes, major feature additions, significant architecture changes
- **Y (Minor)** - New features, non-breaking enhancements, new capabilities
- **Z (Patch)** - Bug fixes, small improvements, configuration tweaks

### Examples:
- `3.0.0` → `3.1.0` = Added new feature (minor bump)
- `3.0.0` → `4.0.0` = Breaking change or major redesign (major bump)
- `3.0.1` → `3.0.2` = Fixed a bug (patch bump)

---

## [3.1.1] - October 30, 2025

### Fixed
- **DS18B20 Temperature Sensor** - Fixed -127.0°C error by increasing conversion delay from 50ms to 800ms for 12-bit resolution (750ms required by datasheet)
- **Initial Temperature Reading** - Fixed setup() temperature reading delay from 200ms to 800ms
- **Heater Manual Override Priority** - Fixed temperature control logic to respect manual overrides with highest priority
- Temperature-controlled heater now properly yields to manual `/control` API commands

### Changed
- Manual override now has absolute highest priority over temperature control for all appliances
- Heater temperature control logic only applies when no manual override is active

### Technical Details
- **DS18B20 Conversion Times**: 9-bit (94ms), 10-bit (188ms), 11-bit (375ms), 12-bit (750ms)
- **Override Priority Order**: Manual Override > Temperature Control > Scheduled Control
- **Backwards Compatible**: No breaking changes to API or configuration

---

## [3.0.1] - October 28, 2025

### Added
- **OLED Display Rotation Support** - Added `display.setRotation(1)` to support rotated hardware configurations
- **Comprehensive Documentation** - Complete README with system analysis, architecture, and API reference
- **Changelog** - This file for version tracking and history

### Changed
- Updated main documentation with detailed system analysis
- Enhanced code header with version history section

### Fixed
- Hardware positioning support for upside-down or rotated displays

### Technical Details
- **Rotation Levels:** 0° (normal), 90° (clockwise), 180° (upside-down), 270° (counter-clockwise)
- **Backwards Compatible:** No breaking changes, rotation defaults to 90°

---

## [3.0.0] - October 21, 2025

### Added
- **6-Device Control System** - Support for Filter, CO2, Light, Heater, HangOnFilter, WaveMaker
- **Intelligent Scheduling** - Minute-based (0-1439) custom schedules per appliance
- **Temperature-Based Heater Control** - Automatic 25-29°C target zone with 30-min minimum runtime
- **Three-Tier Control Priority** - Temperature Control > Manual Override > Scheduled Control
- **REST API (9 Endpoints)**:
  - `/status` - System status and appliance states
  - `/control` - Manual appliance override with timeout support
  - `/schedules` - Bulk schedule management
  - `/schedule` - Single appliance schedule update
  - `/wifi` - WiFi configuration and connection
  - `/emergency/reset` - Emergency shutdown reset
  - `/reset` - Override reset or factory reset
  - `/restart` - System reboot
- **WiFi Management** - Automatic AP mode fallback ("Fishtank_Setup") with NTP synchronization
- **Emergency Safety System** - Auto-shutdown on extreme temperatures (>32°C or <15°C) with 30-min auto-recovery
- **OLED Display** - 0.96" 128x64 status monitor with real-time appliance state
- **RTC with NTP Sync** - DS3231 for accurate timekeeping, NTP sync every 3 hours
- **Temperature Monitoring** - DS18B20 sensor with error detection and recovery
- **Audio Feedback** - Buzzer alerts for system events
- **Memory Optimization** - PROGMEM strings, StaticJsonDocument, reduced update frequencies
- **Professional Protection** - Fuses, TVS diodes, reverse polarity protection, EMI suppression
- **NVS Storage** - Persistent schedules and WiFi credentials

### Technical Specifications
- **Microcontroller**: ESP32 (38-pin)
- **Relays**: 8-channel 12V optocoupler module (Active LOW)
- **Max Appliances**: 6 devices
- **API Port**: 80 (HTTP)
- **Temperature Range**: -20°C to +85°C (sensor)
- **Response Time**: <200ms for relay control
- **Memory**: 520KB RAM, 4MB Flash
- **Default API Key**: `Automate@123`

### Features
- ✅ Commercial product design
- ✅ Indian electrical standards (240V AC, 50Hz)
- ✅ Professional circuit protection
- ✅ Graceful error handling and recovery
- ✅ Non-blocking architecture (millis-based)
- ✅ Automatic WiFi reconnection
- ✅ Factory reset capability
- ✅ Comprehensive logging in DEBUG mode

---

## [2.x] - Previous Versions

### Features
- 5-device control system
- Basic scheduling (hours/minutes format)
- Simple REST API
- Temperature monitoring
- WiFi connectivity

---

## Future Roadmap

### Planned for v3.1.0
- [ ] Web dashboard interface (HTML/CSS/JS)
- [ ] Mobile app integration
- [ ] Advanced scheduling with recurring patterns
- [ ] Multiple timezone support
- [ ] Data logging and analytics


---

## Contributing

When making changes, please:

1. **Update Version Number** in:
   - `esp32-scheduler.ino` (line 3, in header comments)
   - `README.md` (version history section)
   - `CHANGELOG.md` (this file)

2. **Update Date** to current date (format: `Month DD, YYYY`)

3. **Document Changes** following the format:
   - **Added** - New features
   - **Changed** - Modifications to existing features
   - **Fixed** - Bug fixes
   - **Removed** - Deprecations or deletions

4. **Test Thoroughly** before committing:
   - Compile without errors
   - Test all affected features
   - Verify backward compatibility

---

## Version Bump Decision Guide

| Scenario | Bump |
|----------|------|
| Added new appliance support | Major (X) |
| New REST API endpoint | Minor (Y) |
| New configuration option | Minor (Y) |
| Bug fix in relay logic | Patch (Z) |
| Display rotation feature | Patch (Z) |
| Documentation updates | Patch (Z) |
| API response format change | Major (X) |
| Temperature threshold adjustment | Patch (Z) |
| Memory optimization | Patch (Z) |

---

**Maintained by**: mhd-riaz  
**Project**: Arduino - ESP32 Fish Tank Automation  
**Repository**: https://github.com/mhd-riaz/Arduino
