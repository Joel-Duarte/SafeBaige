#ifndef SAFETY_SYSTEMS_H
#define SAFETY_SYSTEMS_H

#include "RadarConfig.h"

class SafetySystems {
private:
    unsigned long _camOffTime = 0;
    const unsigned long _holdTime = 5000; // 5s recording buffer
    const uint8_t _triggerDist = 50;      // 50m YOLO Activation Threshold
    bool _isCamOn = false;

public:
    void init() {
        pinMode(CAM_LIGHT_PIN, OUTPUT);
        digitalWrite(CAM_LIGHT_PIN, LOW);
    }

    bool isRecording() { 
        return _isCamOn; 
    }

    /**
     * @brief Updates safety hardware based on Radar and YOLO feedback
     * @param carDetected Boolean from RadarParser
     * @param closestDist Current closest car distance in meters
     * @param yoloVeto True if Smartphone/YOLO confirmed a False Positive
     */
    void update(bool carDetected, uint8_t closestDist, bool yoloVeto) {
        
        // --- 1. ACTIVATION LOGIC ---
        // Trigger if Radar sees a car within 50m AND YOLO has NOT vetoed it
        if (carDetected && closestDist <= _triggerDist && !yoloVeto) {
            _isCamOn = true;
            _camOffTime = millis() + _holdTime;
            digitalWrite(CAM_LIGHT_PIN, HIGH);
        } 
        
        // --- 2. VETO & SHUTDOWN LOGIC ---
        // Turn OFF if:
        // a) YOLO explicitly identifies a False Positive (Veto)
        // b) The safety hold timer has expired AND no car is in the trigger zone
        bool timeoutExpired = (millis() > _camOffTime);
        bool outOfZone = (!carDetected || closestDist > _triggerDist);

        if (_isCamOn && (yoloVeto || (timeoutExpired && outOfZone))) {
            _isCamOn = false;
            digitalWrite(CAM_LIGHT_PIN, LOW);
            
            if (yoloVeto) {
                Serial.println("SYS: Veto Received - Killing Alert");
            } else {
                Serial.println("SYS: Camera/Light Standby (Timeout)");
            }
        }
    }
};

#endif
