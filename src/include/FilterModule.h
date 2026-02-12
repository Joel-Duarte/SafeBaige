#ifndef FILTER_MODULE_H
#define FILTER_MODULE_H

#include <Arduino.h>

#define FILTER_WINDOW 6 // Smooth over the last 6 frames

class SignalFilter {
private:
    float _history[5][FILTER_WINDOW]; // 5 targets, 3 frames each
    int _index[5] = {0, 0, 0, 0, 0};
    bool _isInitialized[5] = {false, false, false, false, false}; 

public:
    float smooth(int targetId, float newDist) {
        if (targetId >= 5) return newDist;

        // prime the filter with the first value if it's not initialized
        if (!_isInitialized[targetId]) {
            for (int i = 0; i < FILTER_WINDOW; i++) {
                _history[targetId][i] = newDist;
            }
            _isInitialized[targetId] = true;
            return newDist;
        }
        // Add to circular buffer
        _history[targetId][_index[targetId]] = newDist;
        _index[targetId] = (_index[targetId] + 1) % FILTER_WINDOW;

        // Calculate Average
        float sum = 0;
        for (int i = 0; i < FILTER_WINDOW; i++) {
            sum += _history[targetId][i];
        }
        return sum / FILTER_WINDOW;
    }

    void reset(int targetId) {
        _isInitialized[targetId] = false; 
        _index[targetId] = 0;
        for (int i = 0; i < FILTER_WINDOW; i++) _history[targetId][i] = 0;
    }
};

#endif