
# HLK-LD2451 Radar Chip Simulator

A Wokwi-based emulator for the HLK-LD2451 millimeter-wave radar sensor.

## Features

- **Traffic Wave Simulation**: Automatically generates "waves" of 1 to 5 cars at a time to simulate real road conditions.
- **Simultaneous Tracking**: Full implementation of the HLK-LD2451 multi-target protocol (tracks up to 5 targets in a single frame).
- **Physics Engine**: Realistic distance-over-time calculation based on randomized vehicle speeds (40km/h - 120km/h).
- **Binary Protocol Accuracy**: Mocks the 10Hz hardware reporting cycle using official 24GHz FMCW frames.
- **Configurable Registers**: Supports virtual hardware settings for range, sensitivity, and direction filtering.


## Configuration Registers

| Register | Range | Default | Description |
|----------|-------|---------|-------------|
| `direction_filter` | 0-2 | 0 | 0=Both, 1=Approach, 2=Away |
| `max_distance` | 10-100m | 100 | Maximum detection range |
| `min_speed` | 1-20 km/h | 5 | Minimum speed threshold |
| `sensitivity` | 1-15 | 5 | Affects re-trigger delay (Lower = Faster) |

## UART Protocol

**Configuration Frame (ESP32 -> Radar)**:
- **Header**: `FD FC FB FA`
- **Command Word**: Offset 6 (e.g., `FF 00` to Enable Config)
- **Footer**: `04 03 02 01`

**Reporting Frame (Radar -> ESP32)**:
- **Header**: `F4 F3 F2 F1`
- **Payload**: Target Count, Distance, Speed, Direction, SNR
- **Footer**: `F8 F7 F6 F5`

### TODO: Maybe implement driver breaking / slowing down on approach for a more realistic simulation (probably not needed given the current goals)