#include "include/RadarConfig.h"
#include "include/RadarParser.h"
#include "include/SafetySystems.h"
#include "include/DisplayModule.h"
#include "include/NetworkManager.h"

RadarTarget activeTargets[5];
SafetySystems safety;
DisplayModule ui;
NetworkManager network;

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
    int count = RadarParser::parse(Serial2, activeTargets, 5);
    bool phoneAttached = network.isConnected();
    bool cameraRecording = safety.isRecording(); // Added a getter to SafetySystems

    if (count > 0) {
        lastCarSeenTime = millis();
        alreadyClear = false;
        
        uint8_t closest = 100;
        for(int i=0; i<count; i++) {
            if(activeTargets[i].distance < closest) closest = activeTargets[i].distance;
        }
        
        ui.render(count, activeTargets, phoneAttached, cameraRecording);
        safety.update(true, closest);
    } 
    else {
        if (millis() - lastCarSeenTime > CLEAR_TIMEOUT) {
            safety.update(false, 100);
            if (!alreadyClear) {
                ui.showClear(phoneAttached);
                alreadyClear = true;
            }
        }
    }
}
