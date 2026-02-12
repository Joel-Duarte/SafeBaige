#ifndef RADAR_CONFIG_H
#define RADAR_CONFIG_H

#include <Arduino.h>

// Pins
const int RAD_RX = 25; // Matching main.cpp
const int RAD_TX = 27; // Matching main.cpp
const int CAM_LIGHT_PIN = 23;

// Protocol Constants
const uint8_t RADAR_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
const uint8_t RADAR_FOOTER[] = {0xF8, 0xF7, 0xF6, 0xF5};

struct RadarTarget {
    int8_t angle;
    uint8_t distance;
    bool approaching;
    uint8_t speed;
    uint8_t snr;
};

#endif