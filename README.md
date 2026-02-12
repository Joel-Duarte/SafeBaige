# SafeBaige
A little project heavily inspired by garmin varia capabilities, and expanding on it trought the use of YOLO (not implented as this is just an emulated PoC)

## System Architecture
- **Radar Core**: Uses a self made hlk_ld2451 emulator that it interfaces with via UART (115200 baud) to track up to 5 simultaneous targets just like the real thing.
- **Visual Dashboard**: This emulated version uses a SSD1306 to draw a ui with vertical "Radar tracking" with approaching car dots and distance bars.
- **Safety Logic**: Camera/light control, ready(ish) for future camera module implementation to start recording once it detects a car enter a 50m zone, which then has it tested against the yolo model for falso positives and to start periodic recording that's later deleted as "nothing bad happened".
- **IoT Connectivity**: 
    -*station mode*: bridges to a smartphone, or others via the [Wokwi gateway](https://github.com/wokwi/wokwigw)
    -*json api*: servers live data at `http://localhost:9080/data`

## Project Structure

| File | Responsibility |
| ------------- | ------------- |
| RadarParser.h | Decodes the emulated binary HLK-LD2451 Protocol. |
| SafetySystems.h | Manages the camera/light logic. |
| DisplayModule.h | Renders the vertical track, bike icon (triangle), "REC" status, "Phone" connectivity status |
| FilterModule.h | Signal Smoothing filter to remove emulated jitter. |
| NetworkManager.h | Handles the ESPAsyncWebServer and API. |

## Hardware mapping


| Component | ESP32 Pin | Protocol |
| ------------- | ------------- | ------------- |
| Radar TX | GPIO 25 | UART |
| Radar RX | GPIO 27 | UART |
| OLED SDA | GPIO 21 | I2C |
| OLED SCL | GPIO 22 | I2C |
| Camera LED | GPIO 23 | Digital Out |

## IoT API Endpoints

- **GET /data**: Returns live JSON of all tracked vehicles (Distance, Speed, TTC).
- **GET /config**: Remotely configures radar range, sensitivity, direction to track and min speed.
- **GET /yolo_feedback?detected=0**: Allows the phone to "Veto" a radar target if no car is seen by the camera in order to turn the camera off to save power.
- **GET /profile?side=left/right**: Switches ui to show the given "hand" side of traffic for when displaying the "passing" ui.

## Installation / Usage
- compile with pio run
- compile the radarchipemu with wokwi-cli
- Start wokwi simulator

<img width="730" height="539" alt="screenshot-2026-02-12_20-32-44" src="https://github.com/user-attachments/assets/6ff1c2da-52d7-483b-b67d-fe0fef76cb8a" />


