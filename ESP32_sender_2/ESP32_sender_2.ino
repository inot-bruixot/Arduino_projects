/*
  ESP32_sender.ino
  ----------------
  Sends a random number (0–100) every 5 seconds to a specific ESP32
  using the ESP-NOW protocol (no Wi-Fi router needed).

  Wiring: nothing extra — just power via microUSB.

  Before flashing:
    - Replace RECEIVER_MAC with the actual MAC address of your receiver ESP32.
*/

#include <esp_now.h>   // ESP-NOW library
#include <WiFi.h>      // Required to initialize Wi-Fi (ESP-NOW runs on top of it)
#include <OneWire.h>          // Low-level communication protocol
#include <DallasTemperature.h> // Higher-level DS18B20 driver

// ── Configuration ────────────────────────────────────────────────────────────

// MAC address of the receiver ESP32 (change this to match your board)
//uint8_t RECEIVER_MAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
//uint8_t RECEIVER_MAC[] = {0x88, 0x57, 0x21, 0x8E, 0x97, 0xCC};
uint8_t RECEIVER_MAC[] = {0x28, 0x05, 0xA5, 0x74, 0xAF, 0x70};

//MAC Address ESP32 BIG : 88:57:21:8E:97:CC
//MAC Address ESP32 SMALL : 28:05:A5:74:AF:70

// How often to send a number (milliseconds)
#define SEND_INTERVAL 1000

// ---------- Pin definition ----------
#define SENSOR_PIN 14          // GPIO14 = YELLOW wire from the sensor

// ---------- Library setup ----------
OneWire           oneWire(SENSOR_PIN);  // Create a OneWire bus on GPIO14
DallasTemperature sensors(&oneWire);    // Pass the bus to the sensor library

// ── Data structure ────────────────────────────────────────────────────────────

// The package we will send — just one float
typedef struct {
  float number;
} DataPackage;

DataPackage myData;

// ── Callback: called after each send attempt ──────────────────────────────────

// Note: ESP32 core v3.x changed the first parameter from (const uint8_t *mac)
//       to (const wifi_tx_info_t *info) — use the new signature to avoid compiler error
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  
  // --------------------------------------------------------------------
  // Communication :  ESP32 NOW
  // --------------------------------------------------------------------
  
  // Set ESP32 to Station mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the send callback
  esp_now_register_send_cb(onDataSent);

  // Register the receiver as a peer
  esp_now_peer_info_t peerInfo = {};              // zero-initialize the struct
  memcpy(peerInfo.peer_addr, RECEIVER_MAC, 6);   // set MAC address
  peerInfo.channel = 0;                          // 0 = same channel as current
  peerInfo.encrypt = false;                      // no encryption

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESP32 Sender ready.");
  // --------------------------------------------------------------------
  // Communication :  ESP32 NOW
  // --------------------------------------------------------------------

  // --------------------------------------------------------------------
  // Sensor : Enabling the sensor
  // --------------------------------------------------------------------


  // Seed the random number generator using floating analog pin noise
  //randomSeed(analogRead(0));

  // Activating the Sensor
  sensors.begin();
  Serial.println("DS18B20 temperature sensor ready.");
  Serial.println("-----------------------------------");
}

// ── Loop ──────────────────────────────────────────────────────────────────────

void loop() {

  // Ask the sensor to take a temperature measurement
  sensors.requestTemperatures();

  // Read the result from the first (and only) sensor on the bus
  // getC = get temperature in Celsius
  float temperatureC = sensors.getTempCByIndex(0);

 
  // Pick a random number between 0 and 100
  //myData.number = random(0, 101);
  myData.number = temperatureC;
  
    // Start the sensor library
  
  // Send it to the receiver
  esp_now_send(RECEIVER_MAC, (uint8_t *)&myData, sizeof(myData));

  Serial.print("Temperature sent to Centralized ESP32 : ");
  Serial.println(myData.number);

  // Wait 5 seconds before sending the next one
  delay(SEND_INTERVAL);
}
