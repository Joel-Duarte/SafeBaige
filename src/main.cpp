#include "include/RadarConfig.h"
#include "include/RadarParser.h"
#include "include/SafetySystems.h"
#include "include/DisplayModule.h"
#include "include/NetworkManager.h"

RadarTarget activeTargets[5];
SafetySystems safety;
DisplayModule ui;
NetworkManager network;
volatile int globalTargetCount = 0;
unsigned long lastValidRadarTime = 0;
const int DATA_PERSIST_MS = 250;
bool yoloVetoActive = false;
volatile float lastVetoDistance = 0.0f;
bool pendingConfigChange = false;
uint8_t nextRange, nextDir, nextMinSpd, nextSens;

TrafficSide currentTrafficSide = RIGHT_HAND_DRIVE;


unsigned long lastCarSeenTime = 0;
const int CLEAR_TIMEOUT = 500;
bool alreadyClear = false;

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RAD_RX, RAD_TX);
    
    safety.init();
    ui.init();
    network.init();
    Serial.println("Safebaige Modular Boot Complete");
}

void loop() {
  // Check if there's a pending radar configuration change from the web interface
    if (pendingConfigChange) {
        Serial.println("[MAIN] Executing Radar Re-config...");
        
        // Start Config Sequence
        uint8_t start[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
        Serial2.write(start, sizeof(start));
        delay(150);

        // Set Params (0x0002)
        uint8_t params[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x06, 0x00, 0x02, 0x00, nextRange, nextDir, nextMinSpd, 0x01, 0x04, 0x03, 0x02, 0x01};
        Serial2.write(params, sizeof(params));
        delay(150);

        // End Config Sequence
        uint8_t end[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
        Serial2.write(end, sizeof(end));

        pendingConfigChange = false; // Reset the flag
        Serial.println("[MAIN] Config Sequence Finished.");
    }
    int count = RadarParser::parse(Serial2, activeTargets, 5);
    bool phoneAttached = network.isConnected();
    bool cameraRecording = safety.isRecording();

    if (count > 0) {
        lastCarSeenTime = millis();
        globalTargetCount = count;
        alreadyClear = false;
        
        uint8_t closest = 100;
        for(int i = 0; i < count; i++) {
            if(activeTargets[i].distance < closest) closest = activeTargets[i].distance;
        }

        if (yoloVetoActive && abs(closest - lastVetoDistance) > 5) {
            yoloVetoActive = false; 
            Serial.println("YOLO: New target detected. Resetting Veto.");
        }
        ui.render(count, activeTargets, phoneAttached, cameraRecording);
        
        // Pass the 3rd argument: yoloVetoActive
        safety.update(true, closest, yoloVetoActive); 
    } 
    else {
        if (millis() - lastCarSeenTime > DATA_PERSIST_MS) {
            globalTargetCount = 0;
            
            // Pass the 3rd argument: yoloVetoActive
            safety.update(false, 100, yoloVetoActive); 
            
            if (!alreadyClear) {
                ui.showClear(phoneAttached);
                alreadyClear = true;
                yoloVetoActive = false; // Reset veto when road is clear
            }
        }
    }
}
