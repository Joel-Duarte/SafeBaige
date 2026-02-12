#include <Arduino.h>

// Simulated HLK-LD2415H mmWave Radar
#define RADAR_SIM_PIN 14
#define DEBOUNCE_TIME 50

// Function declarations
void onRadarDetection();
void handleRadarEvent();

volatile bool radarDetected = false;
unsigned long lastDebounceTime = 0;

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  Serial.println("Initializing SafeBaige...");
  delay(1000); // Wait for serial to stabilize
  
  // Configure simulated radar sensor pin (button as input)
  pinMode(RADAR_SIM_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RADAR_SIM_PIN), onRadarDetection, FALLING);
  
  Serial.println("SafeBaige - mmWave Radar Car Detection");
  Serial.println("Ready to detect vehicles...");
}

void loop() {
  // Handle radar detection events
  if (radarDetected) {
    radarDetected = false;
    handleRadarEvent();
  }
  delay(10);
}

// Interrupt handler for radar detection (button press simulation)
void onRadarDetection() {
  unsigned long currentTime = millis();
  
  // Simple debounce
  if (currentTime - lastDebounceTime > DEBOUNCE_TIME) {
    radarDetected = true;
    lastDebounceTime = currentTime;
  }
}

// Handle radar detection event
void handleRadarEvent() {
  unsigned long timestamp = millis();
  
  Serial.println("\n========================================");
  Serial.println(">>> [RADAR] DETECTION EVENT TRIGGERED!");
  Serial.print(">>> Timestamp: ");
  Serial.print(timestamp);
  Serial.println(" ms");
  Serial.println("========================================\n");
}