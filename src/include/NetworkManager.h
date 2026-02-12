#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

class NetworkManager {
private:
    AsyncWebServer _server;
    bool _initialized = false;

public:
    // Initialize server on Port 80
    NetworkManager() : _server(80) {}

    void init() {
        Serial.print("Connecting to Wokwi Gateway (Wokwi-GUEST)...");
        
        // Wokwi-GUEST is the required SSID for the bridge to work
        WiFi.begin("Wokwi-GUEST", "", 6); 

        unsigned long startAttempt = millis();
        // Wait for connection (Timeout after 10 seconds)
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            _initialized = true;
            Serial.println("\n[WIFI] Connected to Gateway!");
            Serial.print("[WIFI] Internal IP: ");
            Serial.println(WiFi.localIP()); // Usually 10.13.37.2
        } else {
            Serial.println("\n[ERROR] WiFi Connection Failed. Check Wokwi-Gateway status.");
            return;
        }

        // --- API ENDPOINTS ---

        // 1. Data Endpoint: For the Phone App to fetch target JSON
        _server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
            // In a real app, you'd serialize the activeTargets array here
            request->send(200, "application/json", "{\"status\":\"online\", \"targets\": 0}");
        });

        // 2. Stream Endpoint: Placeholder for the MJPEG/YOLO video feed
        _server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(200, "text/plain", "MJPEG Stream Placeholder - Camera Active");
        });

        _server.begin();
    }

    // Returns true if the ESP32 is successfully on the network
    bool isConnected() {
        return WiFi.status() == WL_CONNECTED;
    }

    // Returns the IP for display on the OLED
    String getIP() {
        if (WiFi.status() == WL_CONNECTED) {
            return WiFi.localIP().toString();
        }
        return "DISCONNECTED";
    }
};

#endif
