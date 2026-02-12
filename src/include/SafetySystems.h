#ifndef SAFETY_SYSTEMS_H
#define SAFETY_SYSTEMS_H

#include "RadarConfig.h"

class SafetySystems {
private:
    unsigned long _camOffTime = 0;
    const unsigned long _holdTime = 5000;
    const uint8_t _triggerDist = 50; // New 50m Activation Threshold
    bool _isCamOn = false;

public:
    bool isRecording() { return _isCamOn; }

public:
    void init() {
        pinMode(CAM_LIGHT_PIN, OUTPUT);
    }

    void update(bool carDetected, uint8_t closestDist) {
        // --- CAMERA & LIGHT TRIGGER LOGIC ---
        // Activate if a car is within 50 meters
        if (carDetected && closestDist <= _triggerDist) {
            _isCamOn = true;
            _camOffTime = millis() + _holdTime;
            digitalWrite(CAM_LIGHT_PIN, HIGH);
        } 
        
        // Handle Shutdown (only if no car is currently within the trigger zone)
        if (_isCamOn && millis() > _camOffTime && (!carDetected || closestDist > _triggerDist)) {
            _isCamOn = false;
            digitalWrite(CAM_LIGHT_PIN, LOW);
            Serial.println("SYS: Camera Standby (Target Cleared Zone)");
        }
        
    }
};

#endif
