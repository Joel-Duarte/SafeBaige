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

        // 1. Position road to the FAR RIGHT
        int roadX = 120;
        _display.drawLine(roadX, 64, roadX, 11, WHITE); 
        _display.fillTriangle(roadX - 3, 10, roadX + 3, 10, roadX, 5, WHITE); 

        for (int i = 0; i < count; i++) {
            int yPos = map(targets[i].distance, 0, 100, 15, 64);
            
            // 2. Map Angle 128 (Center) to 0 offset. 
            // Angle 80 (Passing) will map to a negative offset (Left)
            // We increase the range to -50 to 50 for a wider "lane" feel
            int xOffset = map(targets[i].angle, 0, 255, -80, 80);
            
            if (currentTrafficSide == LEFT_HAND_DRIVE) {
                xOffset = -xOffset;
            }

            // Draw Car Dot
            _display.fillCircle(roadX + xOffset, yPos, 2, WHITE); 
            
            // 3. Distance Bars (Shifted slightly right to fit)
            int barWidth = map(targets[i].distance, 0, 100, 50, 0);
            _display.fillRect(10, 15 + (i * 12), barWidth, 8, WHITE);
            
            _display.setCursor(65, 15 + (i * 12));
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
