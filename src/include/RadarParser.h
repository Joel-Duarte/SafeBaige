#ifndef RADAR_PARSER_H
#define RADAR_PARSER_H

#include "RadarConfig.h"

class RadarParser {
public:
    static int parse(HardwareSerial &ser, RadarTarget *targets, int maxTargets) {
        if (ser.available() < 12) return 0;
        
        if (ser.read() == 0xF4) {
            uint8_t h[3]; ser.readBytes(h, 3);
            if (h[0] == 0xF3 && h[1] == 0xF2 && h[2] == 0xF1) {
                uint16_t dataLen;
                ser.readBytes((uint8_t*)&dataLen, 2);
                uint8_t payload[dataLen];
                ser.readBytes(payload, dataLen);
                uint8_t footer[4];
                ser.readBytes(footer, 4);

                int count = payload[0];
                int actualToRead = (count > maxTargets) ? maxTargets : count;

                for (int i = 0; i < actualToRead; i++) {
                    int base = 2 + (i * 5);
                    // Index 0: Angle, Index 1: Distance, Index 2: Direction, Index 3: Speed, Index 4: SNR
                    targets[i].angle    = payload[base];     // 0-255 (128 is center)
                    targets[i].distance = payload[base + 1]; // 0-100m
                    targets[i].approaching = (payload[base + 2] == 0x01);
                    targets[i].speed    = payload[base + 3];
                    targets[i].snr      = payload[base + 4];
                }
                return actualToRead;
            }
        }
        return 0;
    }
};

#endif
