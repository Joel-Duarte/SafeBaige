#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "RadarConfig.h"

// We pass the global targets to the server
extern volatile int globalTargetCount; 
extern RadarTarget activeTargets[];

class NetworkManager {
private:
    AsyncWebServer _server;

public:
    NetworkManager() : _server(80) {}

    void init() {
        WiFi.begin("Wokwi-GUEST", "", 6);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        // --- DYNAMIC DATA ENDPOINT ---
        _server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
            // Capture the volatile value into a local stack variable
            int currentCount = globalTargetCount; 
            
            String json = "{\"status\":\"online\", \"count\":" + String(currentCount) + ", \"targets\":[";
            
            for(int i = 0; i < currentCount; i++) {
                // Accessing the array directly
                json += "{\"id\":" + String(i) + 
                        ",\"dist\":" + String(activeTargets[i].distance) + "}";
                if(i < currentCount - 1) json += ",";
            }
            json += "]}";
            
            request->send(200, "application/json", json);
        });

        _server.begin();
    }

    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
};

#endif
