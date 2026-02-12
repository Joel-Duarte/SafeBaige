
# HLK-LD2451 Radar Chip Simulator

A Wokwi-based emulator for the HLK-LD2451 millimeter-wave radar sensor.

## Features

- **Radar Simulation**: Simulates approaching and receding vehicles with realistic distance and speed
- **Hardware Registers**: Configurable distance range, speed thresholds, and sensitivity
- **Direction Filtering**: Filter for approaching, receding, or bidirectional motion detection
- **SNR Calculation**: Signal-to-noise ratio based on distance and sensitivity settings
- **UART Interface**: Serial communication for configuration and data output

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

