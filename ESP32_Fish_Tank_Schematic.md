# ESP32 Fish Tank Automation System - Commercial Product KiCad Schematic Design

## Overview
This document provides a complete professional schematic design for the ESP32 Fish Tank Automation System commercial product. Designed for end-user sales with external power supplies and professional installation requirements for PCB design in KiCad.

## Commercial Product System Architecture
The system is designed as a professional control PCB with external customer-provided components:
- **Main Control PCB**: ESP32 Dev Module (38-pin) with integrated protection circuits
- **External Customer-Provided Modules**: OLED, DS3231 RTC, 8-Channel Relay Module
- **Customer Installation**: DS18B20 temperature sensor, buzzer, professional wiring
- **External Power Supplies**: Customer provides SMPS 12V/2A and ERD Charger 5V/2A
- **Professional Protection**: Built-in PCB protection with customer-provided AC protection

---

## 📋 **Commercial Product Component List for KiCad**

### Main Control PCB Components (Built into Product)

| **Component** | **Part Number/Type** | **KiCad Symbol** | **Installation** |
|---------------|---------------------|------------------|------------------|
| U1 | ESP32 DevKit v1 (38-pin) | ESP32-DEVKITV1 | Built into PCB |
| U2 | DS3231 RTC Module | RTC_DS3231 | Customer connects via screw terminals |
| U3 | SSD1306 OLED Module (128x64) | SSD1306_128x64 | Customer connects via screw terminals |
| U4 | 8-Channel Relay Module | RELAY_8CH_MODULE | Customer connects via screw terminals |
| U5 | DS18B20 Temperature Sensor | DS18B20 | Customer installs and connects |
| BZ1 | Buzzer (Active/Passive) | BUZZER | Customer connects |

### Customer-Provided Power & Protection (External to PCB)

| **Component** | **Type/Rating** | **KiCad Symbol** | **Installation** |
|---------------|----------------|------------------|------------------|
| External Power | 5V ERD Charger | N/A | Customer provides (Phone charger type 5V/2A) |
| External Power | 12V SMPS | N/A | Customer provides (12V/2A power supply) |
| Customer MCB | 6A C-Curve MCB | N/A | Customer installs for AC protection |
| J1 | 5V Power Input | CONN_01X02 | PCB Terminal (from Customer ERD Charger) |
| J2 | 12V Power Input | CONN_01X02 | PCB Terminal (from Customer SMPS) |

### Built-in PCB Protection & Power Management

| **Component** | **Part Number/Type** | **KiCad Symbol** | **Installation** |
|---------------|---------------------|------------------|------------------|
| F1 | 2.5A Slow-blow Fuse (5V Rail) | FUSE | Built into PCB |
| F2 | 3A Slow-blow Fuse (12V Rail) | FUSE | Built into PCB |
| D1 | Reverse Polarity Protection | 1N5819 (Schottky, THT) | Built into PCB |
| D2 | TVS Diode (5V Protection) | 1N4744A (15V Zener, THT) | Built into PCB |
| D3 | TVS Diode (12V Protection) | 1N4744A (15V Zener, THT) | Built into PCB |

### Professional PCB Capacitors & Filtering (Built into Product)

| **Component** | **Value/Rating** | **KiCad Symbol** | **Installation** |
|---------------|------------------|------------------|------------------|
| C1 | 470µF/25V (5V Primary Filter) | CP_Large | Built into PCB |
| C3 | 470µF/25V (12V Primary Filter) | CP_Large | Built into PCB |
| C5 | 470µF/25V (5V Secondary) | CP_Large | Built into PCB |

### Professional PCB Resistors & Protection (Built into Product)

| **Component** | **Value/Purpose** | **KiCad Symbol** | **Installation** |
|---------------|-------------------|------------------|------------------|
| R1 | 4.7kΩ (DS18B20 Pull-up) | R | Built into PCB |
| R2 | 4.7kΩ (I2C SDA Pull-up) | R | Built into PCB |
| R3 | 4.7kΩ (I2C SCL Pull-up) | R | Built into PCB |
| R4-R8 | 330Ω (GPIO Relay Protection) | R | Built into PCB |
| R9 | 100Ω (DS18B20 GPIO Protection) | R | Built into PCB |
| R10 | 100Ω (Buzzer GPIO Protection) | R | Built into PCB |
| R11 | 82Ω (I2C SDA Series Protection) | R | Built into PCB |
| R12 | 82Ω (I2C SCL Series Protection) | R | Built into PCB |

### Professional PCB Inductors & EMI Suppression (Built into Product)

| **Component** | **Value/Part Number** | **KiCad Symbol** | **Installation** |
|---------------|----------------------|------------------|------------------|
| FB1 | 120Ω@100MHz/1A (0603 SMD) | Ferrite_Bead | Built into PCB |
| FB2 | 120Ω@100MHz/1A (0603 SMD) | Ferrite_Bead | Built into PCB |
| NTC1 | 5D-11 (10Ω Inrush Limiter) | Thermistor | Built into PCB |

**Alternative Ferrite Beads (if 120Ω unavailable):**
- BLM18PG221SN1D (Murata, 220Ω@100MHz)
- MMZ1608Y121BT000 (TDK equivalent)
- 28A0301-0A0-10 (Fair-Rite through-hole)

### Professional PCB Connection Terminals (Built into Product)

| **Component** | **Pin Count/Purpose** | **KiCad Symbol** | **Installation** |
|---------------|----------------------|------------------|------------------|
| J3 | I2C Connector (4-pin for RTC + OLED) | CONN_01X04 | PCB Screw Terminal |
| J4 | DS18B20 Connector (3-pin) | CONN_01X03 | PCB Screw Terminal |
| J5 | Relay Control Connector (8-pin) | CONN_01X08 | PCB Screw Terminal |
| J6 | Status LEDs Connector | CONN_01X03 | PCB Screw Terminal |
| J7 | Emergency Stop Input | CONN_01X02 | PCB Screw Terminal |
| J8 | Customer Buzzer Output | CONN_01X02 | PCB Screw Terminal |

---

## 🔌 **Commercial Product Professional Pin Connections**

### ESP32 DevKit v1 Pinout (38-pin) - Professional Configuration

| **Pin** | **Left Side** | **Connection** | **Right Side** | **Connection** |
|---------|---------------|----------------|----------------|----------------|
| 1-19 | EN | Not Connected | D23 | Not Connected |
| 2-18 | VP (36) | Not Connected | D22 | I2C SCL (via R3) |
| 3-17 | VN (39) | Not Connected | TXD | Not Connected |
| 4-16 | D34 | Not Connected | RXD | Not Connected |
| 5-15 | D35 | Not Connected | D21 | I2C SDA (via R2) |
| 6-14 | D32 | Not Connected | D19 | Heater Relay (via R7) |
| 7-13 | D33 | Not Connected | D18 | Light Relay (via R6) |
| 8-12 | D25 | Not Connected | D5 | HangOn Filter Relay (via R8) |
| 9-11 | D26 | Not Connected | D17 | CO2 Relay (via R5) |
| 10-10 | D27 | Not Connected | D16 | Filter Relay (via R4) |
| 11-9 | D14 | DS18B20 Data (via R9) | D4 | Not Connected |
| 12-8 | D12 | Not Connected | D0 | Not Connected |
| 13-7 | D13 | Buzzer (via R10) | D2 | Status LED |
| 14-6 | GND | System Ground | D15 | Not Connected |
| 15-5 | VIN | 5V Power Input | GND | System Ground |
| 16-4 | 3V3 | 3.3V Output | 3V3 | 3.3V Output |

### Module Connections

#### DS3231 RTC Module (U2)

| **DS3231 Pin** | **Connection** |
|----------------|----------------|
| VCC | ESP32 3V3 |
| GND | System GND |
| SDA | GPIO 21 (via R11) |
| SCL | GPIO 22 (via R12) |

#### SSD1306 OLED Module (U3)

| **OLED Pin** | **Connection** |
|--------------|----------------|
| VCC | ESP32 3V3 |
| GND | System GND |
| SDA | GPIO 21 (via R11) |
| SCL | GPIO 22 (via R12) |

#### 8-Channel Relay Module (U4)

| **Relay Pin** | **Connection** |
|---------------|----------------|
| VCC | ESP32 5V (VIN) |
| GND | System GND |
| JD-VCC | 12V External Supply |
| IN1 | GPIO 16 (Filter) via R4 |
| IN2 | GPIO 17 (CO2) via R5 |
| IN3 | GPIO 5 (Light) via R6 |
| IN4 | GPIO 19 (Heater) via R7 |
| IN5 | GPIO 18 (HangOn Filter) via R8 |
| IN6-IN8 | Not Connected |

#### DS18B20 Temperature Sensor (U5)

| **DS18B20 Pin** | **Connection** |
|-----------------|----------------|
| VDD | ESP32 3V3 |
| GND | System GND |
| DQ | GPIO 14 (via R9) + Pull-up R1 to 3V3 |

---

## ⚡ **Power Distribution Network**

### 5V Rail (ESP32 Supply)
```
5V Input (J1) → F1 (2.5A Fuse) → D1 (Reverse Protection) → D2 (TVS) → C1 (Filtering) → ESP32 VIN
                                                          ↓
                                                      C5 (Secondary Filter)
```

### 12V Rail (Relay Supply)
```
12V Input (J2) → F2 (3A Fuse) → D3 (TVS) → C3 (Filtering) → Relay Module JD-VCC
```

### 3.3V Rail (Logic & Sensors)
```
ESP32 3V3 → C7||C8 (Decoupling) → RTC Module, OLED Module, DS18B20
```

---

## 🛡️ **Circuit Protection Implementation**

### Input Protection
```
AC Mains (240V) → [MCB: 6A C-Curve] → External Power Supplies → Your Control PCB

External Power Supply Chain:
- ERD Charger 5V/2A → [F1: 2.5A Fuse] → [D2: Zener] → ESP32 System
- SMPS 12V/2A → [F2: 3A Fuse] → [D3: Zener] → Relay Module

AC Appliance Control via Relays:
- Relay1 → Filter (20W/0.08A)
- Relay2 → CO2 (15W/0.06A)
- Relay3 → LED Light (40W/0.17A)  
- Relay4 → Heater (200W/0.83A)
- Relay5 → HangOn Filter (20W/0.08A)

Total AC Load: ~1.22A (well under 6A MCB rating)

Commercial Product Features:
- External SMPS & ERD (customer provides)
- Control PCB with protection
- Professional relay control
- Modular design for easy assembly
```

### GPIO Protection
```
ESP32 GPIO → [330Ω Resistor] → Relay Input
ESP32 GPIO → [100Ω Resistor] → Sensor Input
```

### I2C Protection
```
ESP32 I2C → [82Ω Series] → [4.7kΩ Pull-up] → Modules
```

---

## 📐 **KiCad Schematic Pages**

### Page 1: Main Controller & Power
- ESP32 DevKit v1 with power connections
- Power input connectors and protection
- TVS diodes and fuses
- Primary filtering capacitors

### Page 2: I2C Bus & Sensors
- DS3231 RTC Module connections
- SSD1306 OLED Module connections
- DS18B20 temperature sensor with pull-up
- I2C bus protection and pull-ups

### Page 3: Relay Interface
- 8-Channel Relay Module connections
- GPIO protection resistors
- 12V power supply for relays
- Relay output terminals

### Page 4: Status & Audio
- Buzzer circuit
- Status LED connections
- Test points for debugging

---

## 🔧 **KiCad Symbol Library Requirements**

### Standard KiCad Symbols
- **ESP32-DEVKITV1**: MCU module symbol
- **CONN_01X02**: 2-pin connector
- **CONN_01X03**: 3-pin connector
- **CONN_01X04**: 4-pin connector
- **CONN_01X08**: 8-pin connector
- **FUSE**: Fuse symbol
- **D_Schottky**: Schottky diode
- **D_TVS**: TVS diode symbol
- **R**: Resistor symbol
- **C**: Capacitor symbol
- **CP_Large**: Polarized capacitor

### Custom Symbols Needed
- **DS3231_MODULE**: RTC module (4-pin: VCC, GND, SDA, SCL)
- **SSD1306_MODULE**: OLED module (4-pin: VCC, GND, SDA, SCL)
- **RELAY_8CH_MODULE**: 8-channel relay module
- **DS18B20**: Temperature sensor (3-pin: VDD, GND, DQ)
- **BUZZER**: Buzzer symbol

---

## 📋 **PCB Layout Guidelines**

### Layer Stack-up (4-layer recommended)
- **Layer 1**: Component placement and routing
- **Layer 2**: Ground plane
- **Layer 3**: Power planes (5V, 3.3V, 12V)
- **Layer 4**: Additional routing and signals

### Component Placement
1. **ESP32 Module**: Center of board
2. **Power Section**: Left side with proper heat dissipation
3. **I2C Modules**: Top area, close to ESP32
4. **Relay Interface**: Right side with isolation
5. **Protection Components**: Near input connectors

### Routing Guidelines
- **Power Traces**: Minimum 1mm width for 5V/12V
- **Signal Traces**: 0.2mm minimum width
- **I2C Traces**: Keep short, use 0.3mm width
- **Ground Plane**: Solid pour with minimal splits
- **Isolation**: Maintain 2mm between 12V and logic circuits

### Via Specifications
- **Standard Vias**: 0.2mm drill, 0.5mm pad
- **Power Vias**: 0.3mm drill, 0.7mm pad
- **Via Stitching**: Every 5mm on power planes

---

## 🔌 **Connector Specifications**

### J1: 5V Power Input
```
Pin 1: +5V Input
Pin 2: GND
Connector: Phoenix Contact MKDS 1,5/2
```

### J2: 12V Relay Power
```
Pin 1: +12V Input
Pin 2: GND
Connector: Phoenix Contact MKDS 1,5/2
```

### J3: I2C Bus (RTC + OLED)
```
Pin 1: 3V3
Pin 2: GND
Pin 3: SDA
Pin 4: SCL
Connector: JST XH 2.54mm 4-pin
```

### J4: DS18B20 Temperature Sensor
```
Pin 1: 3V3
Pin 2: GND
Pin 3: Data (GPIO 14)
Connector: JST XH 2.54mm 3-pin
```

### J5: Relay Control Interface
```
Pin 1: Filter (GPIO 16)
Pin 2: CO2 (GPIO 17)
Pin 3: Light (GPIO 5)
Pin 4: Heater (GPIO 19)
Pin 5: HangOn Filter (GPIO 18)
Pin 6: Buzzer (GPIO 13)
Pin 7: Status LED (GPIO 2)
Pin 8: GND
Connector: JST XH 2.54mm 8-pin
```

---

## 📏 **Physical Dimensions**

### PCB Size Recommendations
- **Dimensions**: 100mm x 80mm (fits standard enclosure)
- **Thickness**: 1.6mm (standard FR4)
- **Copper Weight**: 1oz (35µm) for standard, 2oz for power layers

### Module Mounting
- **ESP32 DevKit**: Pin header socket (19-pin x 2)
- **Modules**: Pin headers for removable mounting
- **Relay Module**: Separate mounting with cable connection

---

## 🧪 **Testing Points**

### Power Test Points
- **TP1**: 5V Rail (after protection)
- **TP2**: 3.3V Rail (ESP32 output)
- **TP3**: 12V Rail (relay power)
- **TP4**: Ground reference

### Signal Test Points
- **TP5**: I2C SDA line
- **TP6**: I2C SCL line
- **TP7**: DS18B20 data line
- **TP8**: Buzzer output

---

## 🛠️ **Assembly Notes**

### Component Orientation
- **ESP32**: USB connector towards front edge
- **Electrolytic Capacitors**: Positive terminal marked clearly
- **Diodes**: Cathode band orientation critical
- **Fuses**: Accessible for replacement

### Soldering Sequence
1. Surface mount components (if any)
2. Through-hole resistors and capacitors
3. Diodes and protection components
4. Connectors and headers
5. ESP32 socket last

### Testing Sequence
1. Visual inspection for shorts
2. Power rail continuity
3. Protection circuit verification
4. ESP32 module insertion
5. Functional testing with software

---

## 📁 **KiCad Project Structure**

```
ESP32_FishTank_PCB/
├── ESP32_FishTank.pro (Project file)
├── ESP32_FishTank.sch (Main schematic)
├── ESP32_FishTank.kicad_pcb (PCB layout)
├── Libraries/
│   ├── ESP32_Modules.lib (Custom symbols)
│   ├── FishTank_Footprints.pretty/ (Custom footprints)
│   └── 3D_Models/ (3D models for rendering)
├── Datasheets/
│   ├── ESP32-DevKitV1.pdf
│   ├── DS3231.pdf
│   ├── SSD1306.pdf
│   └── Protection_Components.pdf
└── Documentation/
    ├── BOM.csv (Bill of Materials)
    ├── Assembly_Drawing.pdf
    └── Fabrication_Notes.pdf
```

---

## 💰 **Bill of Materials (BOM)**

### PCB Components
| Designator | Part Number | Description | Quantity | Supplier |
|------------|-------------|-------------|----------|----------|
| U1 | ESP32-DevKitV1 | ESP32 Development Board | 1 | Generic |
| F1 | T2.5AL250V | 2.5A Slow-blow Fuse | 1 | Littelfuse |
| F2 | T3AL250V | 3A Slow-blow Fuse | 1 | Littelfuse |
| D1 | 1N5819 | Schottky Diode 3A/40V | 1 | ON Semi |
| D2 | 1N4744A | Zener Diode 15V 1W THT | 1 | ON Semi |
| D3 | 1N4744A | Zener Diode 15V 1W THT | 1 | ON Semi |
| C1,C3,C5 | 470µF/25V | Electrolytic Capacitor | 3 | Panasonic |
| R1,R2,R3 | 4.7kΩ/0.25W | Resistor 1% | 3 | Yageo |
| R4-R8 | 330Ω/0.25W | Resistor 1% | 5 | Yageo |
| R9,R10 | 100Ω/0.25W | Resistor 1% (GPIO Protection) | 2 | Yageo |
| R11,R12 | 82Ω/0.25W | Resistor 1% (I2C Protection) | 2 | Yageo |
| FB1,FB2 | 120Ω@100MHz/1A | Ferrite Bead 0603 SMD | 2 | Generic |

### External Modules
| Component | Description | Interface | Supplier |
|-----------|-------------|-----------|----------|
| DS3231 RTC Module | Real-time Clock with Crystal | I2C | Generic |
| SSD1306 OLED | 128x64 OLED Display | I2C | Generic |
| 8CH Relay Module | 12V Optocoupler Relays | GPIO | Generic |
| DS18B20 | Waterproof Temperature Sensor | OneWire | Maxim |
| Buzzer | Active/Passive Buzzer 5V | GPIO | Generic |

---

## 🎯 **Design Validation Checklist**

### Electrical Design
- [ ] Power supply protection implemented
- [ ] All GPIO pins have current limiting resistors
- [ ] I2C pull-ups present and correct values
- [ ] TVS diodes on all power rails
- [ ] Reverse polarity protection on inputs
- [ ] Proper grounding and isolation

### Mechanical Design
- [ ] Component spacing adequate for assembly
- [ ] Connector accessibility for maintenance
- [ ] Mounting holes for enclosure
- [ ] Heat dissipation considerations
- [ ] Cable management and strain relief

### Manufacturing
- [ ] Minimum trace width/spacing rules followed
- [ ] Via sizes within PCB fab capabilities
- [ ] Component footprints verified
- [ ] Silkscreen clear and readable
- [ ] Assembly drawings complete

---

This comprehensive schematic design will enable you to create a professional PCB for your ESP32 Fish Tank Automation System in KiCad. The modular approach allows for easy assembly, maintenance, and upgrades while providing robust circuit protection for the Indian electrical environment.