# ESP32 Fish Tank Automation Commercial Product - Professional Connection Diagram

## Commercial Installation Reference Guide

This document provides professional connection diagrams for the ESP32 Fish Tank Automation Commercial Product. These diagrams are designed for qualified electrician installation and customer setup with external power supplies.

---

## 🔌 **ESP32 DevKit v1 (38-pin) - Commercial Product Configuration**

```
                           ESP32 DevKit v1 (38-pin) - Commercial PCB
                     ┌─────────────────────────────────┐
              EN  1  ├○                               ○┤ 38  5V (VIN out)
          VP/A0   2  ├○                               ○┤ 37  CMD  
          VN/A3   3  ├○                               ○┤ 36  D3
         D34/A6   4  ├○                               ○┤ 35  D1
         D35/A7   5  ├○                               ○┤ 34  D2 ──→ Status LED (J6)
         D32/A4   6  ├○    ┌─────────────────┐       ○┤ 33  D0
         D33/A5   7  ├○    │                 │       ○┤ 32  GND
        D25/A18   8  ├○    │     ESP32       │       ○┤ 31  3V3
        D26/A19   9  ├○    │    WROOM-32     │       ○┤ 30  D23
        D27/A17  10  ├○    │                 │       ○┤ 29  D22/SCL ──→ I2C SCL (J3)
        D14/A16  11  ├○    └─────────────────┘       ○┤ 28  TX0
        D12/A15  12  ├○                               ○┤ 27  RX0  
        D13/A14  13  ├○     [USB Connector]           ○┤ 26  D21/SDA ──→ I2C SDA (J3)
            GND  14  ├○                               ○┤ 25  D19 ──→ Heater Relay (J5)
    VIN(J1) 15  ├○       Customer ERD 5V         ○┤ 24  D18 ──→ Light Relay (J5)
            3V3  16  ├○                               ○┤ 23  D5  ──→ HangOn Filter Relay (J5)
            GND  17  ├○                               ○┤ 22  D17 ──→ CO2 Relay (J5)
        D15/A13  18  ├○                               ○┤ 21  D16 ──→ Filter Relay (J5)
         D2/A12  19  ├○                               ○┤ 20  D4
                     └─────────────────────────────────┘

Professional PCB Connections:
• Pin 11 (D14): DS18B20 Temperature Sensor Data → J4 Terminal
• Pin 13 (D13): Customer Buzzer Control → J8 Terminal
• Pin 15 (VIN):  Customer ERD Charger 5V Input → J1 Terminal
• Pin 26 (D21): I2C SDA (Customer RTC + OLED) → J3 Terminal
• Pin 29 (D22): I2C SCL (Customer RTC + OLED) → J3 Terminal
• Pin 21 (D16): Filter Relay Control → J5 Terminal
• Pin 22 (D17): CO2 Relay Control → J5 Terminal
• Pin 24 (D18): Light Relay Control → J5 Terminal
• Pin 25 (D19): Heater Relay Control → J5 Terminal
• Pin 23 (D5):  HangOn Filter Relay Control → J5 Terminal
• Pin 19 (D2):  Status LED → J6 Terminal
```

---

## 🔗 **I2C Bus Connection (Shared)**

```
ESP32 Pin 26 (D21/SDA) ────┬── 82Ω ──┬── DS3231 SDA
                           │         │
                           │         └── SSD1306 SDA
                           │
                           └── 4.7kΩ pull-up to 3V3

ESP32 Pin 29 (D22/SCL) ────┬── 82Ω ──┬── DS3231 SCL
                           │         │
                           │         └── SSD1306 SCL
                           │
                           └── 4.7kΩ pull-up to 3V3

3V3 Supply ────────────────┬── DS3231 VCC
                           │
                           └── SSD1306 VCC

GND ───────────────────────┬── DS3231 GND
                           │
                           └── SSD1306 GND
```

---

## 🌡️ **DS18B20 Temperature Sensor Connection**

```
ESP32 Pin 11 (D14) ───── 100Ω ───── DS18B20 DQ (Data)
                                        │
                                        │
3V3 ─────── 4.7kΩ pull-up ──────────────┘

3V3 ────────────────────────────────── DS18B20 VDD

GND ────────────────────────────────── DS18B20 GND

Waterproof DS18B20 Cable Colors:
• Red Wire    = VDD (3V3)
• Black Wire  = GND  
• Yellow Wire = DQ (Data to GPIO 14)
```

---

## 🔊 **Buzzer Connection**

```
```
ESP32 Pin 13 (D13) ───── 100Ω ───── Buzzer Positive (+)

GND ─────────────────────────────── Buzzer Negative (-)

Note: For ESP32 Core 3.0+, tone() function is used
      For ESP32 Core 2.x, LEDC PWM is used
```

---

## ⚡ **8-Channel Relay Module Connection**

```
Relay Module Control Inputs:
ESP32 Pin 21 (D16) ───── 330Ω ───── IN1 (Filter)
ESP32 Pin 22 (D17) ───── 330Ω ───── IN2 (CO2)
ESP32 Pin 24 (D18) ───── 330Ω ───── IN3 (Light)
ESP32 Pin 25 (D19) ───── 330Ω ───── IN4 (Heater)
ESP32 Pin 23 (D5)  ───── 330Ω ───── IN5 (HangOn Filter)
                                    IN6 (Not Used)
                                    IN7 (Not Used)
                                    IN8 (Not Used)

Relay Module Power:
ESP32 VIN (5V) ──────────────────── Relay VCC
GND ────────────────────────────── Relay GND

External 12V Supply ───────────────── Relay JD-VCC
External 12V GND ──────────────────── Relay GND (Isolated)

Relay Outputs (Each relay has):
• COM (Common)
• NO (Normally Open)  
• NC (Normally Closed)

Note: Remove VCC-JD-VCC jumper for opto-isolation
```

---

## 🔋 **Commercial Product Power Distribution System**

```
Customer ERD Charger 5V Input (External Supply):
Customer 5V ERD ──► J1 PCB Terminal ──► [F1: 2.5A Fuse] ──► [D1: 1N5819 Reverse Protection] ──► [D2: 1N4744A TVS] ──┬── ESP32 VIN
                                                                                                                      │
Customer GND     ─────────────────────────────────────────────────────────────────────────────────────────────────┼── ESP32 GND
                                                                                                                      │
                                                                                                                      ├── Built-in Filter Capacitors:
                                                                                                                      │   C1: 470µF/25V
                                                                                                                      │   C2: 100nF/50V
                                                                                                                      │   C5: 470µF/25V
                                                                                                                      │   C6: 10nF/50V

Customer SMPS 12V Input (External Supply):
Customer 12V SMPS ──► J2 PCB Terminal ──► [F2: 3A Fuse] ──► [D3: 1N4744A TVS] ──┬── J5 Terminal (Customer Relay Module JD-VCC)
                                                                                  │
Customer 12V GND  ───────────────────────────────────────────────────────────────┼── J5 Terminal (Customer Relay Module GND)
                                                                                  │
                                                                                  ├── Built-in Filter Capacitors:
                                                                                  │   C3: 470µF/25V
                                                                                  │   C4: 100nF/50V

3.3V Internal (ESP32 Onboard Regulator):
ESP32 3V3 ──┬── DS3231 VCC
            │
            ├── SSD1306 VCC  
            │
            ├── DS18B20 VDD
            │
            └── Decoupling Capacitors:
                C7-C12: 100nF/50V (Near each module)
```

---

## 🛡️ **Protection Circuit Details**

```
5V Rail Protection:
5V+ ───[F1]───[D1]───[D2]───[C1||C2]───[C5||C6]─── Clean 5V to ESP32
       2.5A   1N5819  1N4744A Primary    Secondary
       Fuse   Schottky 15V     Filter     Filter

12V Rail Protection:  
12V+ ───[F2]───[D3]───[C3||C4]─── Clean 12V to Relays
        3A     1N4744A Primary
        Fuse   15V     Filter

GPIO Protection (Each control line):
ESP32 GPIO ───[330Ω]─── Relay Input
ESP32 GPIO ───[100Ω]─── Sensor Input
```
```

I2C Protection:
ESP32 SDA ───[82Ω]───[4.7kΩ to 3V3]─── I2C SDA Bus
ESP32 SCL ───[82Ω]───[4.7kΩ to 3V3]─── I2C SCL Bus
```

---

## 🔌 **Connector Specifications**

### J1: 5V Power Input (Phoenix Contact)
```
Pin 1: +5V Input (Red Wire)
Pin 2: GND (Black Wire)
```

### J2: 12V Relay Power (Phoenix Contact)
```
Pin 1: +12V Input (Red Wire)  
Pin 2: GND (Black Wire)
```

### J3: I2C Modules (JST XH 4-pin)
```
Pin 1: 3V3 (Red Wire)
Pin 2: GND (Black Wire)
Pin 3: SDA (Blue Wire)
Pin 4: SCL (Yellow Wire)
```

### J4: DS18B20 Temperature (JST XH 3-pin)
```
Pin 1: 3V3 (Red Wire)
Pin 2: GND (Black Wire)  
Pin 3: DQ Data (Yellow Wire)
```

### J5: Control Outputs (JST XH 8-pin)
```
Pin 1: Filter Control (GPIO 16)
Pin 2: CO2 Control (GPIO 17)
Pin 3: Light Control (GPIO 18)
Pin 4: Heater Control (GPIO 19)
Pin 5: HangOn Control (GPIO 5)
Pin 6: Buzzer Output (GPIO 13)
Pin 7: Status LED (GPIO 2)
Pin 8: GND Reference
```

---

## 📏 **PCB Layout Zones**

```
┌─────────────────────────────────────────────────────────────┐
│                    100mm x 80mm PCB Layout                 │
├─────────────────────────────────────────────────────────────┤
│  Power Zone          │         ESP32 Zone        │ Output  │
│  (Left 20mm)         │        (Center 40mm)      │ Zone    │
│                      │                           │(Right   │
│  [F1] [F2]          │                           │ 20mm)   │
│  [D1] [D2] [D3]     │        ┌─────────┐       │         │
│  [C1] [C3]          │        │         │       │ [J5]    │
│  [C2] [C4]          │        │  ESP32  │       │ Control │
│  [C5] [C6]          │        │ DevKit  │       │ Outputs │
│                      │        │   v1    │       │         │
│  [J1] 5V Input      │        │         │       │ [BZ1]   │
│  [J2] 12V Input     │        └─────────┘       │ Buzzer  │
│                      │                           │         │
│                      │  I2C Zone (Top 15mm)     │         │
│                      │  ┌──────┐  ┌──────┐     │         │
│                      │  │ RTC  │  │ OLED │     │         │
│                      │  │ J3   │  │ J3   │     │         │
│                      │  └──────┘  └──────┘     │         │
│                      │                           │         │
│                      │  Sensor Zone (Bottom)    │         │
│                      │  [J4] DS18B20            │         │
│                      │  [R1] Pull-up            │         │
└─────────────────────────────────────────────────────────────┘

Layer Stack (4-layer):
• Layer 1 (Top):    Components + Signal Routing
• Layer 2 (Inner):  Ground Plane (Solid Pour)
• Layer 3 (Inner):  Power Planes (+5V, +3V3, +12V)
• Layer 4 (Bottom): Additional Routing + Test Points
```

---

## 🧪 **Test Points for Debugging**

```
TP1: +5V Rail (After protection) ────── [Test Point]
TP2: +3V3 Rail (ESP32 output) ──────── [Test Point]
TP3: +12V Rail (Relay power) ───────── [Test Point]
TP4: GND Reference ──────────────────── [Test Point]
TP5: I2C SDA Signal ─────────────────── [Test Point]
TP6: I2C SCL Signal ─────────────────── [Test Point]
TP7: DS18B20 Data Signal ────────────── [Test Point]
TP8: Buzzer Output ───────────────────── [Test Point]

Test Point Placement:
• Power test points on left edge
• Signal test points near respective circuits
• Ground test points at corners
• All test points accessible with probe
```

---

## 🔧 **Assembly Sequence**

```
Step 1: Power Components
├── Install fuses F1, F2
├── Install protection diodes D1, D2, D3
├── Install filter capacitors C1-C6
└── Test power rails with multimeter

Step 2: Protection Resistors
├── Install GPIO protection resistors R4-R8
├── Install sensor protection resistors R9-R10
├── Install I2C protection resistors R11-R12
└── Install pull-up resistors R1-R3

Step 3: Connectors
├── Install power input connectors J1, J2
├── Install module connectors J3, J4
├── Install output connector J5
└── Test continuity

Step 4: ESP32 Module
├── Install pin headers for ESP32 socket
├── Insert ESP32 DevKit v1 module
├── Apply power and test 5V → 3V3 conversion
└── Program and test basic GPIO functions

Step 5: Module Connections
├── Connect DS3231 RTC module via J3
├── Connect SSD1306 OLED via J3 (parallel)
├── Connect DS18B20 sensor via J4
└── Test I2C communication and sensor readings

Step 6: Customer Installation & Final Assembly
├── Customer connects relay module via J5 terminal
├── Customer installs buzzer via J8 terminal
├── Customer connects external 12V SMPS to relay JD-VCC via J5
├── Customer connects external 5V ERD charger via J1
├── Professional electrician installs 6A MCB for AC protection
└── Complete system functional test by qualified technician
```

This comprehensive commercial product connection diagram provides all the professional installation references needed for creating your KiCad schematic, PCB layout, and customer installation guide! 🚀