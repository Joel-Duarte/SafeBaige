#include <Arduino.h>

// HLK-LD2451 Binary Protocol Constants
const uint8_t HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
const uint8_t FOOTER[] = {0xF8, 0xF7, 0xF6, 0xF5};

void setup() {
  // Wokwi Serial Monitor (USB)
  Serial.begin(115200);
  
  // Radar Connection (RX=25, TX=27)
  Serial2.begin(115200, SERIAL_8N1, 25, 27);
  
  Serial.println("--- ESP32 Bicycle Radar Online ---");
  Serial.println("Waiting for targets from Custom Chip...");
}

void loop() {
  if (Serial2.available() >= 12) {
    if (Serial2.read() == 0xF4) { // Header Start
      uint8_t header[3];
      Serial2.readBytes(header, 3); // Read F3 F2 F1

      uint16_t dataLen;
      Serial2.readBytes((uint8_t*)&dataLen, 2);

      uint8_t payload[dataLen];
      Serial2.readBytes(payload, dataLen);

      // Read Trailer (4 bytes)
      uint8_t trailer[4];
      Serial2.readBytes(trailer, 4);

      // --- DATA EXTRACTION ---
      uint8_t targetCount = payload[0];
      
      if (targetCount > 0) {
        // Target 1 Data Block:
        // payload[2] = Angle
        // payload[3] = Distance <--- THIS ONE
        // payload[4] = Direction
        // payload[5] = Speed
        
        uint8_t distance = payload[3]; 
        uint8_t speed = payload[5];

        // --- PRINT TO CONSOLE ---
        Serial.print(">>> CAR DETECTED | Distance: ");
        Serial.print(distance);
        Serial.print("m | Speed: ");
        Serial.print(speed);
        Serial.println(" km/h");

        // IoT Logic: Alert if closer than 10 meters
        if (distance < 10) {
          Serial.println("!!! WARNING: CLOSE PROXIMITY !!!");
        }
      }
    }
  }
}