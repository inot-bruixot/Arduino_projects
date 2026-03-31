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

// ── Configuration ────────────────────────────────────────────────────────────

// MAC address of the receiver ESP32 (change this to match your board)
uint8_t RECEIVER_MAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// How often to send a number (milliseconds)
#define SEND_INTERVAL 5000

// ── Data structure ────────────────────────────────────────────────────────────

// The package we will send — just one integer
typedef struct {
  int number;
} DataPackage;

DataPackage myData;

// ── Callback: called after each send attempt ──────────────────────────────────

void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

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

  // Seed the random number generator using floating analog pin noise
  randomSeed(analogRead(0));
}

// ── Loop ──────────────────────────────────────────────────────────────────────

void loop() {
  // Pick a random number between 0 and 100
  myData.number = random(0, 101);

  // Send it to the receiver
  esp_now_send(RECEIVER_MAC, (uint8_t *)&myData, sizeof(myData));

  Serial.print("Sent number: ");
  Serial.println(myData.number);

  // Wait 5 seconds before sending the next one
  delay(SEND_INTERVAL);
}
