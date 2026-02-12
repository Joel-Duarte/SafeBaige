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
    // 1. Try to parse a radar frame (Returns > 0 if cars are found)
    int count = RadarParser::parse(Serial2, activeTargets, 5);
    bool phoneAttached = network.isConnected();
    bool cameraRecording = safety.isRecording();

    if (count > 0) {
        // --- NEW DATA RECEIVED ---
        lastCarSeenTime = millis(); // Refresh the "Persistence" timer
        globalTargetCount = count;  // Update the global for the Webserver
        alreadyClear = false;       // Reset the UI state machine
        
        uint8_t closest = 100;
        for(int i = 0; i < count; i++) {
            if(activeTargets[i].distance < closest) closest = activeTargets[i].distance;
        }
        
        // Update OLED and Safety System (Lights/Camera)
        ui.render(count, activeTargets, phoneAttached, cameraRecording);
        safety.update(true, closest);
    } 
    else {
        // --- NO DATA IN SERIAL BUFFER (BETWEEN FRAMES) ---
        
        // Only clear the data if we haven't seen a car for 250ms
        if (millis() - lastCarSeenTime > DATA_PERSIST_MS) {
            
            // 1. Update Global for Webserver (Phone will now see 0)
            globalTargetCount = 0;
            
            // 2. Update Safety Systems
            safety.update(false, 100);
            
            // 3. Update OLED (Only once to prevent I2C flicker)
            if (!alreadyClear) {
                ui.showClear(phoneAttached);
                alreadyClear = true;
            }
        }
        // NOTE: If we are under 250ms, we DO NOTHING. 
        // The last known car distances stay in 'activeTargets' and 'globalTargetCount'
        // so the Webserver (Phone App) receives a smooth, non-flickering stream.
    }
}
