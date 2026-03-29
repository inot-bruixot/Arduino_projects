// ============================================================
// ESP32 - Print time every 10 seconds to Serial Monitor
// Platform: Arduino IDE
// ============================================================
// This program tracks how much time has passed since the ESP32
// was powered on or reset, and prints the current time
// (in Hour:Minute:Second format) every 10 seconds.
// ============================================================


// --- SETTINGS ---

// How often to print the time (in milliseconds)
// 10 seconds = 10,000 milliseconds
const unsigned long PRINT_INTERVAL = 10000;


// --- GLOBAL VARIABLES ---

// Stores the last time we printed (in milliseconds since boot)
unsigned long lastPrintTime = 0;


// ============================================================
// SETUP — runs once when the ESP32 starts
// ============================================================
void setup() {

  // Start the Serial Monitor at 115200 baud rate
  // (make sure your Serial Monitor is set to the same speed)
  Serial.begin(115200);

  // Small delay to let the serial connection stabilize
  delay(1000);

  Serial.println("ESP32 Clock started!");
  Serial.println("Time will be printed every 10 seconds.");
  Serial.println("------------------------------------");
}


// ============================================================
// LOOP — runs over and over after setup() finishes
// ============================================================
void loop() {

  // millis() returns how many milliseconds have passed since boot
  unsigned long currentMillis = millis();

  // Check if 10 seconds have passed since the last print
  if (currentMillis - lastPrintTime >= PRINT_INTERVAL) {

    // Update the last print time to now
    lastPrintTime = currentMillis;

    // Convert milliseconds into total seconds
    unsigned long totalSeconds = currentMillis / 1000;

    // Break total seconds down into hours, minutes, and seconds
    unsigned long hours   = totalSeconds / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;

    // Print the time in Hour:Minute:Second format
    // padWithZero() makes sure single digits show as "04" instead of "4"
    Serial.print("Time since boot --> ");
    printFormattedTime(hours, minutes, seconds);
  }
}


// ============================================================
// HELPER FUNCTION — prints the time in HH:MM:SS format
// ============================================================
void printFormattedTime(unsigned long h, unsigned long m, unsigned long s) {

  // Print hours with leading zero if needed (e.g. 04 instead of 4)
  if (h < 10) Serial.print("0");
  Serial.print(h);

  Serial.print(":");

  // Print minutes with leading zero if needed
  if (m < 10) Serial.print("0");
  Serial.print(m);

  Serial.print(":");

  // Print seconds with leading zero if needed
  if (s < 10) Serial.print("0");
  Serial.print(s);

  // Move to the next line after printing the time
  Serial.println();
}
