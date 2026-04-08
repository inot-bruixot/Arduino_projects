// ============================================================
//  esp32_NTC.ino
//  Connects the ESP32 to your home WiFi and retrieves the
//  current time from the internet using NTP (Network Time
//  Protocol). The time is then printed to the Serial Monitor
//  every second.
//
//  Board  : ESP32 DevKit V1
//  No extra hardware required — WiFi only.
//
//  HOW IT WORKS (concept):
//  1. The ESP32 connects to your WiFi router.
//  2. It contacts a public NTP server (pool.ntp.org) over the
//     internet and asks: "what time is it right now?"
//  3. The server replies with UTC time (universal reference,
//     no timezone).
//  4. We add a GMT offset (in seconds) to convert UTC to our
//     local timezone.
//  5. From that point the ESP32 tracks time internally.
//     It re-syncs with the NTP server automatically in the
//     background to correct any clock drift.
//
//  LIBRARIES USED (built into the ESP32 Arduino core):
//  - WiFi.h          — connects to your WiFi network
//  - time.h          — standard C time functions (time, localtime_r …)
//  - esp_sntp.h      — ESP32 SNTP client (NTP sync engine)
// ============================================================

#include <WiFi.h>
#include <time.h>
#include <esp_sntp.h>

// ============================================================
//  NETWORK CREDENTIALS
//  Fill these in directly in the Arduino IDE before uploading.
// ============================================================
const char* WIFI_SSID     = "YOUR_WIFI_NAME";      // <-- change this
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";  // <-- change this


// ============================================================
//  TIME CONFIGURATION
//
//  GMT_OFFSET_SEC: difference between your local time and UTC,
//  expressed in seconds.
//  Examples:
//    UTC+0  (UK, Portugal)         -->      0
//    UTC+1  (France, Spain, Italy) -->   3600
//    UTC+2  (Greece, Finland)      -->   7200
//    UTC-5  (New York, EST)        --> -18000
//
//  DAYLIGHT_OFFSET_SEC: extra seconds added during summer time.
//  If your country observes daylight saving time (DST), use 3600.
//  If it does not, use 0.
// ============================================================
const long  GMT_OFFSET_SEC      = 3600;   // <-- adjust to your timezone
const int   DAYLIGHT_OFFSET_SEC = 3600;   // <-- 3600 if DST applies, else 0

// ============================================================
//  NTP SERVER
//  pool.ntp.org is a global cluster of public time servers.
//  You can also use "time.google.com" or "time.cloudflare.com".
// ============================================================
const char* NTP_SERVER = "pool.ntp.org";

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

// ============================================================
//  setup()
//  Runs once when the ESP32 powers on or is reset.
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);   // small pause so the Serial Monitor has time to open

  Serial.println("===========================================");
  Serial.println("  ESP32 NTP Clock");
  Serial.println("===========================================");

  // Step 1 — join the WiFi network
  connectToWiFi();

  // Step 2 — sync the internal clock via NTP
  syncTimeWithNTP();

  Serial.println("-------------------------------------------");
  Serial.println("Clock is running. Printing time every second.");
  Serial.println("-------------------------------------------");
}

// ============================================================
//  loop()
//  Runs repeatedly after setup(). Prints the current time
//  once per second to the Serial Monitor.
// ============================================================
void loop() {
  printCurrentTime();
  delay(1000);   // wait 1 second before printing again
}
