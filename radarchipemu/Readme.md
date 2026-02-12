
# HLK-LD2451 Radar Chip Simulator

A Wokwi-based emulator for the HLK-LD2451 millimeter-wave radar sensor.

## Features

- **Traffic Wave Simulation**: Generates "waves" of 1–5 vehicles with randomized speeds.
- **Simultaneous Tracking**: iplementation of the HLK-LD2451 multi-target protocol (tracks up to 5 targets in a single frame).
- **Overtaking**: Vehicles maintain a "convoy" queue until they reach, where they veer laterally to simulate a passing maneuver.
- **Binary Protocol Accuracy**: Mocks the 10Hz hardware reporting cycle using official 24GHz FMCW frames.
- **Signal Jitter**: Includes simulated sensor noise (+-1) 
- **Configurable Registers**: Supports virtual hardware settings for range, sensitivity, and direction filtering.


## Configuration Registers

| Register | Range | Default | Description |
|----------|-------|---------|-------------|
| `direction_filter` | 0-2 | 0 | 0=Both, 1=Approach, 2=Away |
| `max_distance` | 10-100m | 100 | Maximum detection range / Spawn threshold |
| `min_speed` | 1-20 km/h | 5 | Minimum speed threshold |
| `sensitivity` | 1-15 | 5 | Affects re-trigger delay (Lower = Faster) |

## Binary Protocol Specification

- **Header**: `0xF4 0xF3 0xF2 0xF1`
- **Payload lenght**: 2 bytes (little endian)
- **Target data**: 5-byte blocks per target:
    - *byte 0*: Angle (0-255, 128 is center)
    - *byte 1*: Distance (0-100m)
    - *byte 2*: Direction (0x01: Approaching)
    - *byte 3*: Speed (km/h)
    - *byte 4*: SNR(0-255)
- **Footer**: `0xF8 0xF7 0xF6 0xF5`
