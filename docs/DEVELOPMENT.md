# ESP32 Fish Tank Automation - Development Guide

## 🏗️ **Project Structure**

```
├── src/                          # Source code
│   └── esp32-scheduler/         # Main Arduino sketch
│       ├── esp32-scheduler.ino  # Main sketch (2095 lines)
│       ├── config.h            # Configuration constants
│       └── types.h             # Data structures and types
├── docs/                        # Documentation
├── diagrams/                    # Visual diagrams
├── archive/                     # Legacy code (archived)
├── build.sh                     # Build script
└── README.md                    # Project overview
```

## 🛠️ **Development Setup**

### **Prerequisites**
- Arduino IDE with ESP32 core installed
- Required libraries (see build.sh for list)
- ESP32 DevKit v1 board

### **Quick Build**
```bash
./build.sh
```

### **Manual Build**
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 src/esp32-scheduler/esp32-scheduler.ino
```

## 🔧 **Code Organization**

### **Main Sketch Structure**
1. **Header & Documentation** (Lines 1-100)
2. **Commercial Safety Specs** (Lines 102-210)
3. **Libraries & Compatibility** (Lines 212-248)
4. **Configuration** (Lines 250-318)
5. **Data Structures** (Lines 320-448)
6. **Utility Functions** (Lines 450-548)
7. **Setup Function** (Lines 549-748)
8. **Main Loop** (Lines 750-848)
9. **WiFi Management** (Lines 850-1098)
10. **API Endpoints** (Lines 1100-1298)
11. **Appliance Control** (Lines 1300-1498)
12. **Temperature Control** (Lines 1500-1698)
13. **Display System** (Lines 1700-1898)
14. **Safety Systems** (Lines 1900-1998)
15. **Schedule Management** (Lines 2000-end)

### **Configuration Files**
- `config.h` - All pin definitions, timing constants, thresholds
- `types.h` - Data structures, enumerations, global variable declarations

## 🐛 **Debugging**

### **Enable Debug Mode**
1. Open `config.h`
2. Change `DEBUG_MODE` from `false` to `true`
3. Recompile and upload

### **Serial Monitoring**
```bash
arduino-cli monitor -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32
```

### **Common Issues**
- **WiFi Connection**: Check credentials in NVS
- **Temperature Sensor**: Verify DS18B20 connection and pull-up resistor
- **Relay Control**: Ensure RELAY_ACTIVE_LOW setting matches hardware
- **Memory Issues**: Monitor heap usage in debug mode

## 🚀 **Deployment**

### **Production Build**
- Ensure `DEBUG_MODE = false` in config.h
- Run `./build.sh` for clean production build
- Upload to ESP32 with proper power supplies

### **Configuration**
- Default API key: `Automate@123`
- Access Point: `FishTank_Commercial` / `Commercial123`
- Professional installation required for AC power

## 📊 **Performance Optimization**

### **Memory Usage**
- PROGMEM strings for static text
- Reduced buffer sizes for JSON processing
- Conditional compilation for debug features

### **Timing Optimization**
- Non-blocking operations using `millis()`
- Reduced OLED update frequency (2s intervals)
- Optimized temperature reading intervals

## 🔧 **Maintenance Tasks**

### **Regular Checks**
- [ ] Update library versions
- [ ] Test with different ESP32 cores (2.x/3.x)
- [ ] Verify temperature sensor accuracy
- [ ] Check relay operation with load
- [ ] Validate API security

### **Code Quality**
- [ ] Run static analysis
- [ ] Test edge cases (power loss, sensor failure)
- [ ] Validate timing calculations
- [ ] Check memory usage patterns

## 📝 **Adding New Features**

1. **Define requirements** in documentation
2. **Update configuration** in `config.h`
3. **Add data structures** in `types.h` if needed
4. **Implement functionality** in main sketch
5. **Update API endpoints** if required
6. **Test thoroughly** with hardware
7. **Update documentation**

## 🔒 **Security Considerations**

- API key authentication for all endpoints
- Input validation for all API parameters
- Safe defaults for relay states
- Emergency shutdown on sensor failures
- Professional electrical installation required

## 📞 **Support**

For development issues:
1. Check debug output with `DEBUG_MODE = true`
2. Verify hardware connections against diagrams
3. Test with minimal configuration
4. Review API documentation for correct usage

---

*This guide is maintained alongside the codebase. Last updated: September 28, 2025*