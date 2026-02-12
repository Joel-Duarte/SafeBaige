#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RadarConfig.h"

class DisplayModule {
private:
    Adafruit_SSD1306 _display;

public:
    DisplayModule() : _display(128, 64, &Wire, -1) {}

    void init() {
        if(!_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println("OLED Fail");
        _display.clearDisplay();
        _display.display();
    }

    // Helper to draw the status bar at the top
    void drawStatusBar(bool phoneConnected, bool isRecording) {
        _display.setTextSize(1);
        _display.setTextColor(WHITE);
        
        // WiFi/Phone Icon (Top Left)
        _display.setCursor(0, 0);
        if (phoneConnected) {
            _display.print("PHONE:OK");
        } else {
            _display.print("PHONE:--");
        }

        // Blinking REC Icon (Top Middle)
        if (isRecording) {
            // Blink every 500ms
            if ((millis() / 500) % 2 == 0) {
                _display.fillCircle(70, 3, 3, WHITE);
                _display.setCursor(76, 0);
                _display.print("REC");
            }
        }
    }

    void render(int count, RadarTarget *targets, bool phoneConnected, bool isRecording) {
        _display.clearDisplay();
        drawStatusBar(phoneConnected, isRecording);
        
        // Road Track & Bike (Top Right)
        _display.drawLine(115, 64, 115, 11, WHITE); 
        _display.fillTriangle(112, 10, 118, 10, 115, 5, WHITE); 

        for (int i = 0; i < count; i++) {
            // 0m = Top (15), 100m = Bottom (64)
            int yPos = map(targets[i].distance, 0, 100, 15, 64);
            _display.fillCircle(115, yPos, 3, WHITE); 
            
            // Proximity Bars on Left
            int barLen = map(targets[i].distance, 0, 100, 60, 0); 
            _display.fillRect(5, 15 + (i * 12), barLen, 8, WHITE);
            _display.setCursor(70, 15 + (i * 12));
            _display.print(targets[i].distance); _display.print("m");
        }
        _display.display();
    }
    
    void showClear(bool phoneConnected) {
        _display.clearDisplay();
        drawStatusBar(phoneConnected, false);
        _display.setCursor(35, 30);
        _display.print("ROAD CLEAR");
        _display.display();
    }
};

#endif
