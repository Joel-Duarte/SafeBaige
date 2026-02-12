#include <Arduino.h>

// HLK-LD2451 Binary Protocol Constants
const uint8_t RADAR_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
const uint8_t RADAR_FOOTER[] = {0xF8, 0xF7, 0xF6, 0xF5};

// Tracking Variables
unsigned long lastRadarUpdate = 0;
const int RADAR_TIMEOUT_MS = 500; // If no data for 0.5s, road is clear

void setup() {
  Serial.begin(115200);
  
  // Custom Pins: RX=25, TX=27 (Matches Wokwi Diagram)
  Serial2.begin(115200, SERIAL_8N1, 25, 27);
  
  Serial.println("\n--- SafeBike IoT Radar System Online ---");
}

void loop() {
  // 1. Wait for a full frame (Min 12 bytes for 1 target)
  if (Serial2.available() >= 12) {
    if (Serial2.read() == 0xF4) { // Header Start
      uint8_t h[3];
      Serial2.readBytes(h, 3); // Read F3 F2 F1

      if (h[0] == 0xF3 && h[1] == 0xF2 && h[2] == 0xF1) {
        uint16_t dataLen;
        Serial2.readBytes((uint8_t*)&dataLen, 2);

        uint8_t payload[dataLen];
        Serial2.readBytes(payload, dataLen);

        uint8_t trailer[4];
        Serial2.readBytes(trailer, 4);

        // --- FORMATTED OUTPUT ---
        int targetCount = payload[0];
        
        // Create a single string buffer to prevent broken lines
        String output = "\n[RADAR] Targets: " + String(targetCount) + " | ";

        for (int i = 0; i < targetCount; i++) {
          int base = 2 + (i * 5);
          uint8_t dist  = payload[base + 1];
          uint8_t speed = payload[base + 3];

          output += "C" + String(i + 1) + ": " + String(dist) + "m (" + String(speed) + "km/h)";
          if (i < targetCount - 1) output += " | ";
        }

        // Send the entire line at once
        Serial.println(output);

        // Optional: High Priority Alert on a new line
        if (targetCount > 0 && payload[3] < 15) {
             Serial.println(">>> ! PROXIMITY ALERT ! <<<");
        }
      }
    }
  }
}