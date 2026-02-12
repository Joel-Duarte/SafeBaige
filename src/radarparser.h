#ifndef RADAR_PARSER_H
#define RADAR_PARSER_H

#include <Arduino.h>

struct RadarTarget {
    int8_t angle;
    uint8_t distance;
    bool approaching;
    uint8_t speed;
    uint8_t snr;
};

class Radar {
public:
    static bool readFrame(HardwareSerial &port, RadarTarget &target) {
        if (port.available() < 4) return false;
        
        // Match Header F4 F3 F2 F1
        if (port.read() == 0xF4 && port.peek() == 0xF3) {
            port.read(); port.read(); port.read(); // Consume F3 F2 F1
            
            uint16_t dataLen;
            port.readBytes((uint8_t*)&dataLen, 2);
            
            uint8_t targetCount = port.read();
            uint8_t alarm = port.read();

            if (targetCount > 0) {
                target.angle = (int8_t)port.read() - 128;
                target.distance = port.read();
                target.approaching = (port.read() == 0x01);
                target.speed = port.read();
                target.snr = port.read();

                // Skip extra targets and read trailer
                for (int i = 0; i < (targetCount - 1) * 5 + 4; i++) port.read();
                return true;
            }
        }
        return false;
    }
};

#endif
