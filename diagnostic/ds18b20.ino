/**
 * DS18B20 Diagnostic Test Sketch
 * This will help identify hardware or sensor issues
 * Upload this to ESP32 and check Serial Monitor at 115200 baud
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 14  // GPIO 14

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n========================================");
  Serial.println("DS18B20 DIAGNOSTIC TEST");
  Serial.println("========================================\n");
  
  // Test 1: Check if OneWire bus is working
  Serial.println("TEST 1: OneWire Bus Check");
  Serial.println("-------------------------");
  
  sensors.begin();
  int deviceCount = sensors.getDeviceCount();
  
  Serial.print("Devices found on bus: ");
  Serial.println(deviceCount);
  
  if (deviceCount == 0) {
    Serial.println("❌ ERROR: No devices found!");
    Serial.println("\nPossible issues:");
    Serial.println("  1. Sensor not connected to GPIO 14");
    Serial.println("  2. Missing 4.7kΩ pull-up resistor");
    Serial.println("  3. Loose wiring or bad connections");
    Serial.println("  4. Dead sensor");
    Serial.println("  5. Wrong pin - check GPIO 14");
    Serial.println("\n⚠️ Check your wiring and restart!");
    while(1) { delay(1000); }
  }
  
  Serial.println("✅ Device(s) detected!\n");
  
  // Test 2: Get device address
  Serial.println("TEST 2: Device Address");
  Serial.println("----------------------");
  
  DeviceAddress deviceAddress;
  if (sensors.getAddress(deviceAddress, 0)) {
    Serial.print("Device 0 Address: ");
    for (uint8_t i = 0; i < 8; i++) {
      if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
      if (i < 7) Serial.print(":");
    }
    Serial.println();
    
    // Check if it's a genuine DS18B20 (family code should be 0x28)
    if (deviceAddress[0] == 0x28) {
      Serial.println("✅ Device is DS18B20 (Family Code: 0x28)");
    } else {
      Serial.print("⚠️ WARNING: Unexpected family code: 0x");
      Serial.println(deviceAddress[0], HEX);
      Serial.println("   This might be a fake/counterfeit sensor!");
    }
  } else {
    Serial.println("❌ ERROR: Could not read device address");
  }
  Serial.println();
  
  // Test 3: Read parasite power mode
  Serial.println("TEST 3: Power Mode Check");
  Serial.println("------------------------");
  if (sensors.isParasitePowerMode()) {
    Serial.println("⚠️ Parasite power mode detected");
    Serial.println("   Make sure VDD pin is connected to 3.3V!");
  } else {
    Serial.println("✅ Normal power mode (VDD connected)");
  }
  Serial.println();
  
  // Test 4: Try different resolutions
  Serial.println("TEST 4: Temperature Reading Tests");
  Serial.println("---------------------------------");
  
  Serial.println("\nTesting 9-bit resolution (fastest)...");
  testResolution(9, 100);
  
  Serial.println("\nTesting 10-bit resolution...");
  testResolution(10, 200);
  
  Serial.println("\nTesting 11-bit resolution...");
  testResolution(11, 400);
  
  Serial.println("\nTesting 12-bit resolution (most accurate)...");
  testResolution(12, 800);
  
  Serial.println("\n========================================");
  Serial.println("DIAGNOSTIC COMPLETE");
  Serial.println("========================================");
  Serial.println("\nContinuous monitoring starting...\n");
}

void loop() {
  Serial.println("--- Reading Temperature ---");
  
  sensors.requestTemperatures();
  delay(800);  // Wait for 12-bit conversion
  
  float tempC = sensors.getTempCByIndex(0);
  
  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.println("°C");
  
  if (tempC == -127.0 || tempC == 85.0) {
    Serial.println("❌ INVALID READING!");
    if (tempC == 85.0) {
      Serial.println("   (85°C is sensor power-on default - sensor not responding)");
    }
  } else if (tempC < -55.0 || tempC > 125.0) {
    Serial.println("❌ OUT OF RANGE!");
  } else {
    Serial.println("✅ Valid reading");
  }
  
  Serial.println();
  delay(2000);  // Wait 2 seconds between readings
}

void testResolution(int bits, int delayMs) {
  sensors.setResolution(bits);
  sensors.requestTemperatures();
  delay(delayMs);
  
  float tempC = sensors.getTempCByIndex(0);
  
  Serial.print("  ");
  Serial.print(bits);
  Serial.print("-bit (");
  Serial.print(delayMs);
  Serial.print("ms delay): ");
  Serial.print(tempC);
  Serial.print("°C ");
  
  if (tempC == -127.0) {
    Serial.println("❌ FAILED");
  } else if (tempC == 85.0) {
    Serial.println("⚠️ Power-on default (sensor not responding)");
  } else if (tempC > -55.0 && tempC < 125.0) {
    Serial.println("✅ SUCCESS");
  } else {
    Serial.println("⚠️ Out of range");
  }
}
