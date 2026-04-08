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
#include <time.h>
#include <esp_sntp.h>
#include <esp_wifi.h>  // needed for esp_wifi_set_channel() // update to run together WiFi_NTC and ESP-NOW


// ============================================================
//  NETWORK CREDENTIALS
//  Fill these in directly in the Arduino IDE before uploading.
// ============================================================
const char* WIFI_SSID     = "YOUR_WIFI_NAME";      // <-- change this
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";  // <-- change this


// ============================================================
// NTC Data
// ============================================================
const long  GMT_OFFSET_SEC      = 3600;   // <-- adjust to your timezone
const int   DAYLIGHT_OFFSET_SEC = 3600;   // <-- 3600 if DST applies, else 0
// ============================================================
//  NTP SERVER
// ============================================================
const char* NTP_SERVER = "pool.ntp.org";
// ============================================================
// ============================================================

// ============================================================
// ── Data Received structure ────────────────────────────────────────────────────────────

// Must match the exact same struct used in the sender
typedef struct {
  float number;
} DataPackage;

DataPackage receivedData;


// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

  // Step 1 — set STA mode first, before anything else // update to run together WiFi_NTC and ESP-NOW
  WiFi.mode(WIFI_STA); // update to run together WiFi_NTC and ESP-NOW

  // Step 2 — join the WiFi router // update to run together WiFi_NTC and ESP-NOW
  connectToWiFi(); // update to run together WiFi_NTC and ESP-NOW

  // Step 3 — sync the internal RTC via NTP (time is stored on-chip after this) // update to run together WiFi_NTC and ESP-NOW
  syncTimeWithNTP(); // update to run together WiFi_NTC and ESP-NOW

  // Step 4 — drop the router connection; radio stays on, time is kept in RTC // update to run together WiFi_NTC and ESP-NOW
  WiFi.disconnect(); // update to run together WiFi_NTC and ESP-NOW
  delay(100);

  // Step 5 — force radio back to channel 1 so ESP-NOW sender (default ch1) can reach us // update to run together WiFi_NTC and ESP-NOW
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // update to run together WiFi_NTC and ESP-NOW

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
  
  printCurrentTime();  // update to run together WiFi_NTC and ESP-NOW
  delay(5000);
}


// ============================================================
// FUNCTIONS  
// ============================================================

// ── Callback: called automatically every time data arrives ───────────────────

// Note: ESP32 core v3.x changed the first parameter from (const uint8_t *mac)
//       to (const esp_now_recv_info_t *info) — use the new signature to avoid compiler error
void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  // Copy the raw bytes into our struct
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  // Print the number to Serial Monitor
  Serial.print("The data received is: ");
  Serial.println(receivedData.number);
}


// ============================================================
//  connectToWiFi()
//  Attempts to join the WiFi network. Prints dots to Serial
//  while waiting, then confirms with the assigned IP address.
// ============================================================
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Keep waiting until the connection is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Connection successful
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  // prints the IP assigned by your router
}

// ============================================================
//  syncTimeWithNTP()
//  Asks the ESP32 SNTP client to contact the NTP server and
//  set the internal clock. configTime() handles everything:
//    - timezone offset
//    - daylight saving offset
//    - which NTP server to contact
//  We then wait until the time is actually confirmed as valid.
// ============================================================
void syncTimeWithNTP() {
  Serial.println("Contacting NTP server to get current time...");

  // This single call starts the NTP sync process
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  // Wait until the internal clock is set to a valid time.
  // time(NULL) returns the current Unix timestamp (seconds since
  // Jan 1 1970). Until NTP replies, it returns a very small number.
  // We keep waiting until it looks like a real recent date (> year 2020).
  struct tm timeinfo;
  Serial.print("Waiting for NTP sync");
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Time synchronized successfully!");
}


// ============================================================
//  printCurrentTime()
//  Reads the current local time from the ESP32 internal clock
//  and prints it to the Serial Monitor in a readable format.
// ============================================================
void printCurrentTime() {
  struct tm timeinfo;   // struct that holds hour, minute, second, date, etc.

  // getLocalTime() fills the struct with the current local time.
  // Returns false if the clock is not yet set.
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Error: could not read time. Is NTP synced?");
    return;
  }

  // strftime() formats the time struct into a human-readable string.
  // %A = full weekday name   (e.g. Monday)
  // %d = day of month        (e.g. 06)
  // %B = full month name     (e.g. April)
  // %Y = 4-digit year        (e.g. 2026)
  // %H = hour (24h format)   (e.g. 14)
  // %M = minutes             (e.g. 35)
  // %S = seconds             (e.g. 02)
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%A, %d %B %Y  -  %H:%M:%S", &timeinfo);

  Serial.println(buffer);
}