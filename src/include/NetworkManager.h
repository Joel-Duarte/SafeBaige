#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "RadarConfig.h"

extern volatile int globalTargetCount;
extern bool yoloVetoActive; // Global flag to kill alerts
extern volatile float lastVetoDistance;
extern RadarTarget activeTargets[];


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
        _server.on("/config", HTTP_GET, [this](AsyncWebServerRequest *request){
            
            // enable config mode (stop radar data stream, allow config writes)
            uint8_t enableCfg[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
            sendRadarCommand(enableCfg, sizeof(enableCfg));

            // function 0x0002: Set Tracking Parameters (Max Range, Direction, Min Speed)
            uint8_t range = request->hasParam("range") ? request->getParam("range")->value().toInt() : 100;
            uint8_t dir   = request->hasParam("direction") ? request->getParam("direction")->value().toInt() : 1;
            uint8_t minSpd = request->hasParam("min_speed") ? request->getParam("min_speed")->value().toInt() : 5;
            
            uint8_t setParams[] = {
                0xFD, 0xFC, 0xFB, 0xFA, 0x06, 0x00, 0x02, 0x00, 
                range, dir, minSpd, 0x01, // [MaxDist] [Dir] [MinSpeed] [Delay:1s]
                0x04, 0x03, 0x02, 0x01
            };
            sendRadarCommand(setParams, sizeof(setParams));

            // function 0x0003: Set Sensitivity
            if (request->hasParam("sensitivity")) {
                uint8_t sens = request->getParam("sensitivity")->value().toInt();
                uint8_t setSens[] = {
                    0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x03, 0x00, 
                    sens, 0x00, // [Sensitivity] [Reserved]
                    0x04, 0x03, 0x02, 0x01
                };
                sendRadarCommand(setSens, sizeof(setSens));
            }

            // footer command to end config mode and resume radar data stream
            uint8_t endCfg[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
            sendRadarCommand(endCfg, sizeof(endCfg));

            request->send(200, "text/plain", "Full Radar Config Synced");
        });



        _server.begin();
    }

    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
};

#endif