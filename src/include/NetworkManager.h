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

public:
    NetworkManager() : _server(80) {}

    void init() {
        WiFi.begin("Wokwi-GUEST", "", 6);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        // --- 1. VIDEO STREAM (Placeholder for MJPEG) ---
        _server.on("/mjpeg", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(200, "multipart/x-mixed-replace; boundary=frame", "STREAM_DATA");
        });

        // --- 2. YOLO FEEDBACK ENDPOINT ---
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

        _server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
            // Use a local copy to ensure thread safety
            int countSnapshot = globalTargetCount;
            
            // Reserve memory immediately to prevent fragmentation
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


        _server.begin();
    }

    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
};

#endif