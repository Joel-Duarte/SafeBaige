#ifndef RADAR_CONFIG_H
#define RADAR_CONFIG_H

#include <Arduino.h>

// Pins
const int RAD_RX = 25;
const int RAD_TX = 27;
const int CAM_LIGHT_PIN = 23;

// Traffic Side Logic
enum TrafficSide { LEFT_HAND_DRIVE, RIGHT_HAND_DRIVE };
extern TrafficSide currentTrafficSide; // Declare it exists globally

struct RadarTarget {
    uint8_t angle;      // 0-255 (128 is center)
    uint8_t distance;   // 0-100m
    bool approaching;
    uint8_t speed;
    uint8_t snr;
};

#endif