#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// --- Configuration ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pins
const int CAM_LIGHT_PIN = 23;
const int BUZZER_PIN = 18;
const int BUZZER_CHANNEL = 0;

// Safety Timer Logic
const unsigned long CAMERA_HOLD_TIME = 5000; // 5s recording after car passes
unsigned long cameraOffTime = 0;
bool isCameraActive = false;

void setup() {
  Serial.begin(115200);
  // Radar on Pins 25 (RX) and 27 (TX)
  Serial2.begin(115200, SERIAL_8N1, 25, 27);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.display();

  // Initialize Outputs
  pinMode(CAM_LIGHT_PIN, OUTPUT);
  
  // Legacy LEDC Setup for Buzzer (Low Volume Dev Mode)
  ledcSetup(BUZZER_CHANNEL, 2000, 8); 
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  Serial.println("--- SafeBike System Booted ---");
}

void updateDisplay(uint8_t count, uint8_t* dists) {
  display.clearDisplay();
  
  // Draw Road Track
  display.drawLine(115, 0, 115, 64, WHITE);
  display.fillTriangle(112, 60, 118, 60, 115, 55, WHITE); // Bike Icon

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("RADAR: "); display.print(count);

  for (int i = 0; i < count; i++) {
    // Map 0-100m to screen Y (60 to 0)
    int yPos = map(dists[i], 0, 100, 55, 5);
    display.fillCircle(115, yPos, 3, WHITE); // Draw car on track

    // Distance Bar on Left
    int barLen = map(dists[i], 0, 100, 60, 0);
    display.fillRect(5, 12 + (i * 12), 60 - barLen, 8, WHITE);
    display.setCursor(70, 12 + (i * 12));
    display.print(dists[i]); display.print("m");
  }
  display.display();
}

void loop() {
  bool carDetectedNow = false;
  uint8_t closestDist = 100;

  // 1. Parse Radar Frame
  if (Serial2.available() >= 12) {
    if (Serial2.read() == 0xF4) { // Header check
      uint8_t h[3]; Serial2.readBytes(h, 3);
      uint16_t dataLen; Serial2.readBytes((uint8_t*)&dataLen, 2);
      uint8_t payload[dataLen]; Serial2.readBytes(payload, dataLen);
      uint8_t footer[4]; Serial2.readBytes(footer, 4);

      uint8_t count = payload[0];
      if (count > 0) {
        carDetectedNow = true;
        uint8_t currentDists[count];
        
        for (int i = 0; i < count; i++) {
          currentDists[i] = payload[3 + (i * 5)];
          if (currentDists[i] < closestDist) closestDist = currentDists[i];
        }
        
        updateDisplay(count, currentDists);
        
        // Trigger Camera & Light
        isCameraActive = true;
        cameraOffTime = millis() + CAMERA_HOLD_TIME;
      }
    }
  }

  // 2. Camera & Light Logic
  if (isCameraActive) {
    digitalWrite(CAM_LIGHT_PIN, HIGH);
    if (millis() > cameraOffTime && !carDetectedNow) {
      isCameraActive = false;
      digitalWrite(CAM_LIGHT_PIN, LOW);
      Serial.println("EVENT: Camera Saved & Stopped.");
    }
  }

  // 3. Buzzer Proximity Logic (1% Volume Dev Mode)
  if (carDetectedNow) {
    // Intensity cap at 3 (approx 1% of 255)
    int intensity = map(closestDist, 2, 40, 3, 0); 
    ledcWrite(BUZZER_CHANNEL, constrain(intensity, 0, 3));
  } else {
    ledcWrite(BUZZER_CHANNEL, 0);
    // Clear screen if road is empty
    if (!isCameraActive) {
      display.clearDisplay();
      display.display();
    }
  }
}
