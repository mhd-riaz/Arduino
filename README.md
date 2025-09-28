# ESP32 Fish Tank Automation System

[![Version](https://img.shields.io/badge/version-3.0-blue.svg)](https://github.com/mhd-riaz/Arduino)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Status](https://img.shields.io/badge/status-Production%20Ready-success.svg)]()

A professional, commercial-grade ESP32-based automation system for aquarium management with intelligent scheduling, temperature control, and REST API management.

## 🏗️ **Architecture Overview**

This system is designed as a commercial product with:
- **Professional PCB Design** for end-user sales
- **External Power Supplies** (customer-provided)
- **Modular Components** for easy installation
- **REST API** for remote management
- **Safety Systems** for reliable operation

### **Key Features**

- ✅ **5-Device Control**: Filter, CO2, Light, Heater, Hang-on Filter
- ✅ **Intelligent Scheduling**: Minute-based timing with custom schedules
- ✅ **Temperature Control**: DS18B20 sensor with 25-29°C heater management
- ✅ **WiFi Connectivity**: STA/AP modes with automatic fallback
- ✅ **REST API**: JSON-based remote management with authentication
- ✅ **OLED Display**: Real-time status monitoring
- ✅ **Emergency Safety**: Automatic shutdown on extreme temperatures
- ✅ **Memory Optimized**: Production-ready with DEBUG_MODE=false
- ✅ **Audio Feedback**: Buzzer alerts for system events

## 📁 **Repository Structure**

```
├── src/                          # Source code
│   └── esp32-scheduler/         # Main Arduino sketch
├── docs/                        # Documentation
│   ├── Documentation.md         # REST API reference
│   ├── ESP32_Fish_Tank_Schematic.md  # KiCad design files
│   ├── Connection_Diagram.md    # Installation guide
│   ├── Automation_Prompt.md     # Code generation guide
│   ├── DEVELOPMENT.md           # Development and maintenance guide
│   └── KiCad_Symbol_Library.md  # Custom symbols
├── diagrams/                    # Visual diagrams
│   ├── esp32_pinout_diagram.svg
│   ├── ds18b20_connection_diagram.svg
│   ├── i2c_bus_diagram.svg
│   ├── pcb_layout_zones_diagram.svg
│   └── power_distribution_diagram.svg
├── archive/                     # Legacy code (archived)
├── LICENSE                      # MIT License
└── README.md                    # This file
```

## 🚀 **Quick Start**

### **Hardware Requirements**
- ESP32 DevKit v1 (38-pin)
- DS3231 RTC Module
- SSD1306 OLED Display (0.96", 128x64)
- 8-Channel Relay Module (12V)
- DS18B20 Temperature Sensor
- Buzzer (active/passive)
- External Power: 5V/2A ERD Charger + 12V/2A SMPS

### **Software Setup**
1. **Install Arduino IDE** with ESP32 core
2. **Install Required Libraries**:
   ```bash
   # Via Arduino Library Manager:
   - ESPAsyncWebServer
   - AsyncTCP
   - RTClib
   - OneWire
   - DallasTemperature
   - Adafruit GFX Library
   - Adafruit SSD1306
   - ArduinoJson (v6+)
   - NTPClient
   ```

3. **Upload Code**:
   - Open `src/esp32-scheduler/esp32-scheduler.ino`
   - Select ESP32 Dev Module board
   - Upload to ESP32

4. **Initial Setup**:
   - System starts in AP mode: `FishTank_Commercial`
   - Connect and configure WiFi via web interface
   - Set schedules and preferences

### **Build Options**

#### **Automated Build (Recommended)**
```bash
./build.sh
```

#### **Manual Build**
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 src/esp32-scheduler/esp32-scheduler.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 src/esp32-scheduler/esp32-scheduler.ino
```

#### **Monitor Serial Output**
```bash
arduino-cli monitor -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32
```

## 📡 **API Usage**

```bash
# Get system status
curl -H "X-API-Key: Automate@123" http://esp32-ip/status

# Control appliance
curl -X POST -H "X-API-Key: Automate@123" \
  -H "Content-Type: application/json" \
  -d '{"state": "ON", "duration": 3600}' \
  http://esp32-ip/control/Filter
```

See [📋 REST API Documentation](docs/Documentation.md) for complete reference.

## 🔧 **Configuration**

### **GPIO Pin Assignments**
| Appliance | GPIO Pin | Function |
|-----------|----------|----------|
| Filter | 18 | Main filtration |
| CO2 | 16 | CO2 injection |
| Light | 5 | LED lighting |
| Heater | 19 | Temperature control |
| Hang-on Filter | 17 | Secondary filtration |

### **Default Schedules**
- **Filter**: 24/7 operation
- **CO2**: 8:30 AM - 1:30 PM, 3:30 PM - 8:30 PM
- **Light**: 8:00 AM - 8:00 PM
- **Heater**: Temperature-controlled (25-29°C)
- **Hang-on Filter**: 24/7 operation

## 🛡️ **Safety Features**

- **Temperature Monitoring**: Automatic heater control with sensor failure handling
- **Emergency Shutdown**: Activates at >32°C or <20°C
- **Power Protection**: Built-in fuses and TVS diodes
- **Watchdog Timer**: ESP32 built-in software watchdog
- **Safe Defaults**: All relays OFF on startup

## 📊 **System Specifications**

- **Power Consumption**: ~2W (ESP32) + relay loads
- **Memory Usage**: Optimized for ESP32 (320KB RAM)
- **WiFi**: 2.4GHz b/g/n with automatic reconnection
- **Temperature Range**: 20-32°C (safe operation)
- **Time Accuracy**: ±1 minute (RTC + NTP sync)
- **API Response Time**: <100ms

## 🔌 **Installation Guide**

See [🔌 Connection Diagrams](docs/Connection_Diagram.md) for detailed wiring instructions.

### **Professional Installation Requirements**
- Qualified electrician for AC power setup
- 6A MCB circuit breaker (customer provides)
- Proper earthing connection
- IP65 enclosure for electronics

## �️ **Development**

### **Code Structure**
```
src/esp32-scheduler/
├── esp32-scheduler.ino    # Main sketch (2095 lines)
├── Configuration          # Pin defines & constants
├── Libraries & Includes   # External libraries
├── Data Structures        # Appliance arrays, schedules
├── Core Functions         # Setup, loop, utilities
├── WiFi Management        # Connection & AP mode
├── API Endpoints          # REST API handlers
├── Appliance Control      # Relay & scheduling logic
├── Temperature Control    # DS18B20 & heater management
├── Display System         # OLED interface
└── Safety Systems         # Emergency & watchdog
```

### **Memory Optimization**
- DEBUG_MODE=false for production builds
- PROGMEM string storage
- Reduced buffer sizes
- Conditional compilation for debug features

### **Cross-Platform Compatibility**
- ESP32 Arduino Core 2.x and 3.0+ support
- Automatic API detection for buzzer control
- Compatible with major ESP32 development boards

## 📈 **Roadmap**

- [ ] Mobile app for iOS/Android
- [ ] Advanced scheduling with lunar cycles
- [ ] Multi-tank support
- [ ] Cloud integration
- [ ] Energy monitoring
- [ ] Automated pH control

## 🤝 **Contributing**

1. Fork the repository
2. Create a feature branch
3. Make changes with proper documentation
4. Test thoroughly on ESP32 hardware
5. Submit a pull request

## 📄 **License**

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 📞 **Support**

For commercial inquiries, installation support, or technical assistance:
- 📧 Email: [your-email@example.com]
- 📋 Documentation: [docs/](docs/)
- 🐛 Issues: [GitHub Issues](https://github.com/mhd-riaz/Arduino/issues)

---

**⚡ Professional Aquarium Automation - Built for Reliability**

*Version 3.0 - September 2025*