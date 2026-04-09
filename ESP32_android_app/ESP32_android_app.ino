// ============================================================
//  ESP32_android_app.ino
//  ─────────────────────────────────────────────────────────
//  Serves temperature and time data to an Android app via HTTP.
//
//  What this sketch does:
//    1. Connects to your home WiFi router.
//    2. Syncs the internal clock with an NTP server (internet time).
//    3. Starts a small HTTP server on port 80.
//    4. When the Android app calls GET /data, the ESP32 reads
//       the DS18B20 temperature sensor and replies with JSON:
//         { "temperature": 23.4, "time": "14:35:02" }
//
//  The Android app simply asks for /data every second and
//  displays whatever the ESP32 replies — no other ESP32 needed.
//
//  Board   : ESP32 DevKit V1
//  Wiring  : DS18B20 data wire → GPIO14
//             4.7kΩ resistor between GPIO14 and 3.3V (pull-up)
//
//  Libraries required (install via Arduino Library Manager):
//    - OneWire          (by Paul Stoffregen)
//    - DallasTemperature (by Miles Burton)
//  Built-in (no install needed):
//    - WiFi, WebServer, time, esp_sntp  (part of ESP32 Arduino core)
// ============================================================


// ── Includes ─────────────────────────────────────────────────────────────────

#include <WiFi.h>              // Connects ESP32 to your WiFi router
#include <WebServer.h>         // Creates the HTTP server
#include <time.h>              // Standard C time functions (localtime, strftime…)
#include <esp_sntp.h>          // ESP32 NTP sync engine
#include <OneWire.h>           // Low-level 1-Wire bus protocol
#include <DallasTemperature.h> // DS18B20 sensor driver (sits on top of OneWire)


// ── WiFi credentials ─────────────────────────────────────────────────────────
// Fill these in before flashing. Keep them private — do not commit to GitHub.

const char* WIFI_SSID     = "YOUR_WIFI_NAME";      // <-- change this
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";  // <-- change this



// ── Time configuration ───────────────────────────────────────────────────────
// Same settings as in esp32_NTC.ino (UTC+1, DST applies).

const long  GMT_OFFSET_SEC      = 3600;  // UTC+1 (Spain/France/Italy…)
const int   DAYLIGHT_OFFSET_SEC = 3600;  // +1h extra during summer (DST)
const char* NTP_SERVER          = "pool.ntp.org";


// ── Temperature sensor setup ──────────────────────────────────────────────────
// Same wiring and pin as in ESP32_sender_2.ino.

#define SENSOR_PIN 14                        // GPIO14 = data wire of DS18B20
OneWire           oneWire(SENSOR_PIN);       // 1-Wire bus on GPIO14
DallasTemperature sensors(&oneWire);         // DS18B20 driver on that bus


// ── HTTP server ───────────────────────────────────────────────────────────────
// Port 80 is the standard HTTP port (same as any website).
// The Android app will call:  http://<ESP32-IP>/data

WebServer server(80);


// ── Serial status print ───────────────────────────────────────────────────────
// Every PRINT_INTERVAL milliseconds, the loop prints a status line.
// Uses millis() instead of delay() so the HTTP server is never blocked.

#define PRINT_INTERVAL 5000          // print every 5 seconds (in milliseconds)
unsigned long lastPrintMillis = 0;   // timestamp of the last print


// ── connectToWiFi() ──────────────────────────────────────────────────────────
// Joins the WiFi router. Prints dots while waiting, then shows the IP address.
// Write down the IP printed in the Serial Monitor — you need it in the app.

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());  // e.g. 192.168.1.42  <-- use this in the app
}


// ── syncTimeWithNTP() ─────────────────────────────────────────────────────────
// Contacts the NTP server to set the internal clock.
// Called once in setup(). After this, the ESP32 tracks time internally
// and does NOT need internet anymore.

void syncTimeWithNTP() {
  Serial.println("Contacting NTP server...");

  // configTime sets the timezone offset and starts the sync in the background
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  // Wait until the clock is confirmed valid (getLocalTime returns false until synced)
  struct tm timeinfo;
  Serial.print("Waiting for NTP sync");
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Time synchronized!");
}


// ── printStatus() ────────────────────────────────────────────────────────────
// Reads the sensor and the internal clock, then prints one status line.
// Called from loop() every PRINT_INTERVAL milliseconds.

void printStatus() {

  // --- Read temperature ---
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // --- Read current time ---
  struct tm timeinfo;
  char timeStr[10] = "??:??:??";
  if (getLocalTime(&timeinfo)) {
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  }

  // --- Print status line ---
  Serial.print("Time : ");
  Serial.print(timeStr);
  Serial.print(" // Temperature : ");
  Serial.print(temperature, 1);  // 1 decimal place
  Serial.println(" °C");
}


// ── handleData() ─────────────────────────────────────────────────────────────
// This function runs every time the Android app calls GET /data.
// It reads the sensor, reads the clock, builds a JSON string, and sends it back.

void handleData() {

  // --- Read temperature ---
  sensors.requestTemperatures();              // ask the sensor to measure
  float temperature = sensors.getTempCByIndex(0); // get result in Celsius

  // --- Read current time ---
  struct tm timeinfo;
  char timeStr[10] = "??:??:??"; // fallback if clock somehow not ready
  if (getLocalTime(&timeinfo)) {
    // Format: HH:MM:SS  (e.g. "14:35:02")
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  }

  // --- Build JSON response ---
  // Example output: {"temperature":23.4,"time":"14:35:02"}
  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"time\":\"" + String(timeStr) + "\"";
  json += "}";

  // --- Send response ---
  // "Access-Control-Allow-Origin: *" is required when the app runs in a browser.
  // Without it, the browser blocks the response for security reasons (CORS policy).
  // This header tells the browser: "any page is allowed to read this data."
  // The Android app ignores this header — it only matters for browsers.
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);

  // --- Log to Serial Monitor (useful for debugging) ---
  Serial.print("Request received → sent: ");
  Serial.println(json);
}


// ── setup() ──────────────────────────────────────────────────────────────────
// Runs once when the ESP32 powers on or is reset.

void setup() {
  Serial.begin(115200);
  delay(1000); // small pause so Serial Monitor has time to open

  Serial.println("===========================================");
  Serial.println("  ESP32 Android App Server");
  Serial.println("===========================================");

  // Step 1 — start the temperature sensor
  sensors.begin();
  Serial.println("DS18B20 sensor ready.");

  // Step 2 — connect to WiFi router
  connectToWiFi();

  // Step 3 — sync clock with NTP (only needed once)
  syncTimeWithNTP();

  // Step 4 — define the HTTP route and start the server
  // server.on() maps a URL path to a handler function.
  // When the app calls GET /data, handleData() runs.
  server.on("/data", handleData);
  server.begin();

  Serial.println("-------------------------------------------");
  Serial.println("HTTP server running. Waiting for requests.");
  Serial.println("Open the Serial Monitor to see incoming requests.");
  Serial.println("===========================================");
}


// ── loop() ───────────────────────────────────────────────────────────────────
// Runs continuously after setup().
//
// Two things happen here, independently of each other:
//   1. server.handleClient() → checks for incoming app requests every iteration
//   2. printStatus()         → prints to Serial every 5 seconds (via millis)
//
// Because we use millis() instead of delay(), both can coexist:
// the server is never frozen while waiting for the next print.

void loop() {

  // --- Always: handle incoming HTTP requests from the app ---
  server.handleClient();

  // --- Every 5 seconds: print status to Serial Monitor ---
  // millis() returns how many ms have passed since boot.
  // When the difference exceeds PRINT_INTERVAL, it's time to print.
  if (millis() - lastPrintMillis >= PRINT_INTERVAL) {
    lastPrintMillis = millis();  // reset the stopwatch
    printStatus();
  }
}
