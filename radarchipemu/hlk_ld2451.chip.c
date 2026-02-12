#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uart_dev_t uart;
  float current_dist;
  float speed_mps;
  bool car_active;
  uint64_t next_event_time_ns;
  
  // --- NEW HARDWARE REGISTERS ---
  bool config_enabled;
  uint8_t direction_filter; // 0: Both, 1: Approach, 2: Away
  uint8_t max_distance;     // 10m to 100m
  uint8_t min_speed;        // 1km/h to 20km/h
  uint8_t sensitivity;      // 1 to 15
  // ------------------------------

  uint8_t rx_buffer[32];
  uint8_t rx_idx;
} chip_state_t;

const uint8_t CFG_HEADER[] = {0xFD, 0xFC, 0xFB, 0xFA};
const uint8_t CFG_FOOTER[] = {0x04, 0x03, 0x02, 0x01};

static void process_command(chip_state_t *chip) {
  // Byte 6 of the buffer is the command (0xFF = Enable, 0xFE = Disable)
  uint8_t cmd = chip->rx_buffer[6];

  if (cmd == 0xFF) {
    chip->config_enabled = true;
    printf("[RADAR CHIP] Config Mode: ENABLED\n");
  } else if (cmd == 0xFE) {
    chip->config_enabled = false;
    printf("[RADAR CHIP] Config Mode: DISABLED\n");
  }
}

// UART RX callback
static void on_uart_data(void *user_data, uint8_t byte) {
  chip_state_t *chip = (chip_state_t *)user_data;
  
  if (chip->rx_idx < 32) {
    chip->rx_buffer[chip->rx_idx++] = byte;
  }

  // Check for footer to determine end of command frame
  if (chip->rx_idx >= 4 && memcmp(&chip->rx_buffer[chip->rx_idx - 4], CFG_FOOTER, 4) == 0) {
    if (memcmp(chip->rx_buffer, CFG_HEADER, 4) == 0) {
      process_command(chip);
    }
    chip->rx_idx = 0;
  }
}

// radar data simulation
static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
    // If config mode is enabled, skip radar simulation to allow stable configuration
  if (chip->config_enabled) return;

  uint64_t now_ns = get_sim_nanos();

  // scanning logic
  if (!chip->car_active) {
    if (now_ns > chip->next_event_time_ns) {
      // Randomly decide if the next car is approaching or receding
      bool approaching = (rand() % 2 == 0);

      // hardware filter logic 
      if (chip->direction_filter != 0) {
        uint8_t current_dir = approaching ? 1 : 2;
        if (current_dir != chip->direction_filter) {
          // filter out car
          chip->next_event_time_ns = now_ns + 500000000;
          return; 
        }
      }

      // spawn new car
      chip->car_active = true;
      chip->current_dist = 10.0f + (rand() % (chip->max_distance - 10)); //random distance between 10m and max_distance
      
      float speed_kmh = (float)(chip->min_speed + (rand() % 80)); // Random speed between min_speed and 80 km/h
      chip->speed_mps = speed_kmh / 3.6;

      printf("[RADAR] SPAWN: %s Car @ %0.1f km/h (Dist: %dm)\n", 
              approaching ? "Approaching" : "Receding", speed_kmh, chip->max_distance);
    }
    return;
  }

  // Update distance based on speed 
  chip->current_dist -= (chip->speed_mps * 0.1);

  // pass/lost logic
  if (chip->current_dist <= 1.0 || chip->current_dist > 120.0) {
    chip->car_active = false;
    
    // sensitivity-based reset delay: 2s base + (sensitivity * 300ms). Higher sensitivity = faster respawn.
    uint32_t reset_delay_ms = 2000 + (chip->sensitivity * 300);
    chip->next_event_time_ns = now_ns + (uint64_t)reset_delay_ms * 1000000;
    
    printf("[RADAR] LOST: Target out of range. Scanning in %dms...\n", reset_delay_ms);
    return;
  }

  // Calculate SNR based on distance and sensitivity (simplified model)
  uint8_t snr = (uint8_t)(255 - (chip->current_dist * 2));
  if (snr < 10) snr = 10; // Minimum noise floor

  // Construct radar data frame
  uint8_t frame[] = {
    0xF4, 0xF3, 0xF2, 0xF1, // Header
    0x07, 0x00,             // Payload Length (7 bytes)
    0x01,                   // Target Count: 1
    0x01,                   // Alarm: Active
    0x80,                   // Angle: 0° (128 - 128)
    (uint8_t)chip->current_dist, 
    0x01,                   // Direction: 1 (Approaching)
    (uint8_t)(chip->speed_mps * 3.6), 
    snr,                    // Calculated SNR
    0xF8, 0xF7, 0xF6, 0xF5  // Footer
  };

  uart_write(chip->uart, frame, sizeof(frame));
}


void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  const uart_config_t uart_config = {
    .user_data = chip,
    .rx = pin_init("RX", INPUT_PULLUP),
    .tx = pin_init("TX", OUTPUT), 
    .baud_rate = 115200,
    .rx_data = on_uart_data,
  };
  chip->uart = uart_init(&uart_config);

  // Default Factory Settings (matches real HLK-LD2451)
  chip->config_enabled = false;
  chip->direction_filter = 1; // Default: approach only
  chip->max_distance = 100;   // Default: 100 meters
  chip->min_speed = 5;        // Default: 5 km/h
  chip->sensitivity = 5;      // Default: Mid-range
  
  chip->car_active = false;
  chip->rx_idx = 0;
  chip->next_event_time_ns = get_sim_nanos() + 1000000000;
  // Start timer for radar simulation
  const timer_config_t timer_config = {
    .user_data = chip,
    .callback = on_timer,
  };
  timer_t timer_id = timer_init(&timer_config);
  timer_start(timer_id, 100000, true);
}
