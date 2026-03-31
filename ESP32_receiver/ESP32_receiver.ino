/*
  ESP32_receiver.ino
  ------------------
  Receives a number sent by another ESP32 via ESP-NOW protocol
  and prints it to the Serial Monitor.

  Wiring: nothing extra — just power via microUSB.

  Before flashing:
    - Open Serial Monitor at 115200 baud to see incoming numbers.
    - Flash THIS board first, note its MAC address, then put it in the sender.
*/

#include <esp_now.h>   // ESP-NOW library
#include <WiFi.h>      // Required to initialize Wi-Fi (ESP-NOW runs on top of it)

// ── Data structure ────────────────────────────────────────────────────────────

// Must match the exact same struct used in the sender
typedef struct {
  int number;
} DataPackage;

DataPackage receivedData;

// ── Callback: called automatically every time data arrives ───────────────────

// Note: ESP32 core v3.x changed the first parameter from (const uint8_t *mac)
//       to (const esp_now_recv_info_t *info) — use the new signature to avoid compiler error
void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  // Copy the raw bytes into our struct
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  // Print the number to Serial Monitor
  Serial.print("The number received is: ");
  Serial.println(receivedData.number);
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

  // Set ESP32 to Station mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();  // Explicitly starts the STA interface — needed to get a valid MAC address

  // Print this board's MAC address (useful for configuring the sender)
  Serial.print("Receiver MAC address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the receive callback
  esp_now_register_recv_cb(onDataReceived);

  Serial.println("ESP32 Receiver ready. Waiting for data...");
}

// ── Loop ──────────────────────────────────────────────────────────────────────

void loop() {
  // Nothing needed here — onDataReceived fires automatically when data arrives
}
