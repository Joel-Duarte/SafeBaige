#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "RadarConfig.h"

extern volatile int globalTargetCount;
extern bool yoloVetoActive; // Global flag to kill alerts
extern volatile float lastVetoDistance;
extern RadarTarget activeTargets[];
extern bool pendingConfigChange;
extern uint8_t nextRange, nextDir, nextMinSpd, nextSens;


class NetworkManager {
private:
    AsyncWebServer _server;

    //Helper to send commands to radar (if needed in future)
    void sendRadarCommand(uint8_t* cmd, size_t len) {
        Serial2.write(cmd, len);
        delay(60); // Small delay to ensure radar has time to process command before next one arrives
    }

public:
    NetworkManager() : _server(80) {}

    void init() {
        WiFi.begin("Wokwi-GUEST", "", 6);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        // video stream endpoint (for future use, not implemented in this code)
        _server.on("/mjpeg", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(200, "multipart/x-mixed-replace; boundary=frame", "STREAM_DATA");
        });

        // yolo feedback endpoint
        // Phone calls this: http://10.13.37.2
        _server.on("/yolo_feedback", HTTP_GET, [](AsyncWebServerRequest *request){
            if (request->hasParam("detected")) {
                bool detected = (request->getParam("detected")->value() == "1");
                yoloVetoActive = !detected;
                
                // If it's a false positive, lock the distance
                if (yoloVetoActive && globalTargetCount > 0) {
                    lastVetoDistance = (float)activeTargets[0].distance;
                    Serial.printf("YOLO VETO: Locked at %.1fm\n", lastVetoDistance);
                }
            }
            request->send(200, "text/plain", "ACK");
        });
        
        // Data endpoint for debugging/monitoring (returns JSON with current targets)
        _server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
            int countSnapshot = globalTargetCount;
            
            String json;
            json.reserve(256); 
            
            json = "{\"status\":\"online\",\"count\":";
            json += String(countSnapshot);
            json += ",\"targets\":[";
            
            for(int i = 0; i < countSnapshot; i++) {
                json += "{\"id\":";
                json += String(i);
                json += ",\"dist\":";
                json += String(activeTargets[i].distance);
                json += "}";
                if(i < countSnapshot - 1) json += ",";
            }
            json += "]}";
            
            // Use 'application/json' to help curl/apps parse it
            request->send(200, "application/json", json);
        });

        // Configuration endpoint to set radar parameters (range, direction to track (approaching/receding), sensitivity, min speed)
        _server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
            // 1. Just harvest the data, do NOT use delay() or Serial2.write() here
            nextRange  = request->hasParam("range") ? request->getParam("range")->value().toInt() : 100;
            nextDir    = request->hasParam("direction") ? request->getParam("direction")->value().toInt() : 1;
            nextMinSpd = request->hasParam("min_speed") ? request->getParam("min_speed")->value().toInt() : 5;
            nextSens   = request->hasParam("sensitivity") ? request->getParam("sensitivity")->value().toInt() : 5;
            
            // 2. Set the flag for the main loop to handle
            pendingConfigChange = true; 
            
            request->send(200, "text/plain", "Config Queued for Main Loop");
        });

        _server.on("/profile", HTTP_GET, [](AsyncWebServerRequest *request){
            if (request->hasParam("side")) {
                String side = request->getParam("side")->value();
                currentTrafficSide = (side == "left") ? LEFT_HAND_DRIVE : RIGHT_HAND_DRIVE;
            }

            if (request->hasParam("mode")) {
                String mode = request->getParam("mode")->value();
                if (mode == "city") {
                    nextRange = 30; nextMinSpd = 10; nextSens = 3;
                } else if (mode == "highway") {
                    nextRange = 100; nextMinSpd = 5; nextSens = 8;
                }
                pendingConfigChange = true;
            }
            request->send(200, "text/plain", "Profile Applied");
        });


        _server.begin();
    }

    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
};

#endif