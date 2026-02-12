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
  // 1. DATA ACQUISITION
  if (Serial2.available() >= 12) {
    if (Serial2.read() == 0xF4) { // Look for start of Header
      uint8_t h[3];
      Serial2.readBytes(h, 3); // Read F3 F2 F1

      if (h[0] == 0xF3 && h[1] == 0xF2 && h[2] == 0xF1) {
        uint16_t dataLen;
        Serial2.readBytes((uint8_t*)&dataLen, 2);

        uint8_t payload[dataLen];
        Serial2.readBytes(payload, dataLen);

        uint8_t footer[4];
        Serial2.readBytes(footer, 4); // Read F8 F7 F6 F5

        // 2. DATA PARSING
        uint8_t targetCount = payload[0];
        if (targetCount > 0) {
          // Byte 1: Angle, Byte 2: Distance, Byte 3: Direction, Byte 4: Speed, Byte 5: SNR
          uint8_t distance = payload[3]; 
          uint8_t speed    = payload[5];
          uint8_t snr      = payload[6];
          bool approaching = (payload[4] == 0x01);

          lastRadarUpdate = millis(); // Refresh timeout timer

          // 3. IOT OUTPUT
          Serial.print("TARGET: ");
          Serial.print(distance); Serial.print("m | ");
          Serial.print(speed); Serial.print("km/h | ");
          Serial.print("SNR: "); Serial.print(snr);
          Serial.println(approaching ? " [CLOSING]" : " [PASSING]");

          // Proximity Alert Logic
          if (distance < 15 && approaching) {
            Serial.println(">> ALERT: VEHICLE IN 15m RANGE <<");
          }
        }
      }
    }
  }

  // 4. TIMEOUT LOGIC (Road Status)
  if (millis() - lastRadarUpdate > RADAR_TIMEOUT_MS && lastRadarUpdate != 0) {
    Serial.println("STATUS: Road Clear (No Targets)");
    lastRadarUpdate = 0; // Reset so it doesn't spam "Clear"
  }
}
