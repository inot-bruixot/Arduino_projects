// ============================================================
//  temperature_sensor.ino
//  Reads temperature from a DS18B20 waterproof probe and
//  prints it to the Serial Monitor every second.
//
//  Board  : ESP32 DevKit V1
//  Sensor : DS18B20 waterproof probe (red / black / yellow)
//
//  WIRING:
//  -------
//  DS18B20 RED    wire  -->  ESP32 3.3V  (power)
//  DS18B20 BLACK  wire  -->  ESP32 GND   (ground)
//  DS18B20 YELLOW wire  -->  ESP32 GPIO14 (data)
//
//  IMPORTANT: You MUST place a 4.7kΩ resistor between the
//  RED wire (3.3V) and the YELLOW wire (data). Without it
//  the sensor will not work. This is called a "pull-up"
//  resistor and it is required by the OneWire protocol.
//
//  LIBRARIES REQUIRED (install via Arduino IDE Library Manager):
//  - OneWire        by Paul Stoffregen
//  - DallasTemperature  by Miles Burton
// ============================================================

#include <OneWire.h>          // Low-level communication protocol
#include <DallasTemperature.h> // Higher-level DS18B20 driver

// ---------- Pin definition ----------
#define SENSOR_PIN 14          // GPIO14 = YELLOW wire from the sensor

// ---------- Library setup ----------
OneWire           oneWire(SENSOR_PIN);  // Create a OneWire bus on GPIO14
DallasTemperature sensors(&oneWire);    // Pass the bus to the sensor library

// ============================================================
void setup() {

  // Start Serial communication so we can print to the monitor
  Serial.begin(115200);

  // Start the sensor library
  sensors.begin();

  Serial.println("DS18B20 temperature sensor ready.");
  Serial.println("-----------------------------------");
}

// ============================================================
void loop() {

  // Ask the sensor to take a temperature measurement
  sensors.requestTemperatures();

  // Read the result from the first (and only) sensor on the bus
  // getC = get temperature in Celsius
  float temperatureC = sensors.getTempCByIndex(0);

  // Check if the reading is valid.
  // The library returns -127 when the sensor is not found or wired wrong.
  if (temperatureC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: sensor not found. Check your wiring.");
  } else {
    // Print in the requested format
    Serial.print("The temperature is : ");
    Serial.print(temperatureC, 1);   // 1 decimal place, e.g. 23.5
    Serial.println(" °C");
  }

  // Wait 1 second before the next reading
  delay(1000);
}
