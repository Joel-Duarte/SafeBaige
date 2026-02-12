#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_TARGETS 5 
typedef struct {
  float distance;
  float speed_mps;
  bool active;
} target_t;
typedef struct {
  uart_dev_t uart;
  target_t targets[MAX_TARGETS];
  uint64_t next_wave_time_ns;
  
  bool config_enabled;
  uint8_t direction_filter;
  uint8_t max_distance;
  uint8_t min_speed;
  uint8_t sensitivity;

  uint8_t rx_buffer[32];
  uint8_t rx_idx;
} chip_state_t;

const uint8_t CFG_HEADER[] = {0xFD, 0xFC, 0xFB, 0xFA};
const uint8_t CFG_FOOTER[] = {0x04, 0x03, 0x02, 0x01};

static void process_command(chip_state_t *chip) {
    // 1. Extract the 16-bit Command Word (Indices 6 and 7)
    uint16_t cmd = chip->rx_buffer[6] | (chip->rx_buffer[7] << 8);

    // DEBUG: See exactly what's hitting the parser
    printf("[RADAR] Command Received: 0x%04X (Config State: %s)\n", 
            cmd, chip->config_enabled ? "OPEN" : "LOCKED");

    // 2. Command Logic Switch
    switch (cmd) {
        case 0x00FF: // --- ENABLE CONFIG ---
            chip->config_enabled = true;
            printf("[RADAR] Mode -> CONFIG ENABLED\n");
            break;

        case 0x00FE: // --- DISABLE CONFIG ---
            chip->config_enabled = false;
            printf("[RADAR] Mode -> CONFIG CLOSED\n");
            break;

        case 0x0002: // --- SET DETECTION PARAMS ---
            if (chip->config_enabled) {
                // Byte 8: Max Range, Byte 9: Direction, Byte 10: Min Speed
                chip->max_distance = chip->rx_buffer[8];
                chip->direction_filter = chip->rx_buffer[9];
                chip->min_speed = chip->rx_buffer[10];
                
                printf("[RADAR] UPDATE -> Range: %dm | Dir: %d | MinSpd: %d\n", 
                        chip->max_distance, chip->direction_filter, chip->min_speed);
            } else {
                printf("[WARNING] Command 0x0002 ignored: Config not enabled!\n");
            }
            break;

        case 0x0003: // --- SET SENSITIVITY ---
            if (chip->config_enabled) {
                // Byte 8: Sensitivity (1-15)
                chip->sensitivity = chip->rx_buffer[8];
                printf("[RADAR] UPDATE -> Sensitivity: %d\n", chip->sensitivity);
            } else {
                printf("[WARNING] Command 0x0003 ignored: Config not enabled!\n");
            }
            break;

        default:
            printf("[RADAR] Unknown Command: 0x%04X\n", cmd);
            break;
    }
}

// UART RX callback
static void on_uart_data(void *user_data, uint8_t byte) {
    chip_state_t *chip = (chip_state_t *)user_data;
    if (chip->rx_idx < 32) chip->rx_buffer[chip->rx_idx++] = byte;

    // Check for footer (04 03 02 01)
    if (chip->rx_idx >= 4 && memcmp(&chip->rx_buffer[chip->rx_idx - 4], CFG_FOOTER, 4) == 0) {
        // Only process if the header matches
        if (memcmp(chip->rx_buffer, CFG_HEADER, 4) == 0) {
            process_command(chip);
        }
        // Reset buffer index for next command
        chip->rx_idx = 0; 
    }
}

// radar data simulation
static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->config_enabled) return;

  uint64_t now_ns = get_sim_nanos();
  int active_count = 0;

  // wave spawning logic
  if (now_ns > chip->next_wave_time_ns) {
    int num_to_spawn = 1 + (rand() % 5); // Random 1, 2, 3, 4, or 5
    printf("[RADAR] TRAFFIC WAVE: Spawning %d cars...\n", num_to_spawn);

    for (int i = 0, spawned = 0; i < MAX_TARGETS && spawned < num_to_spawn; i++) {
      if (!chip->targets[i].active) {
        chip->targets[i].active = true;
        // Stagger them slightly so they aren't on top of each other
        chip->targets[i].distance = (float)chip->max_distance + (spawned * 10.0f); // Start just beyond max distance, staggered by 10m
        float speed_kmh = (float)(chip->min_speed + (rand() % 80)); // Random speed between min_speed and 80 km/h
        chip->targets[i].speed_mps = speed_kmh / 3.6f; //Convert to m/s
        spawned++;
      }
    }
    // Wait 3-10 seconds for the next wave
    chip->next_wave_time_ns = now_ns + (uint64_t)(3000 + (rand() % 7000)) * 1000000;
  }

  // data frame construction
  uint8_t frame[64]; 
  int idx = 0;

  frame[idx++] = 0xF4; frame[idx++] = 0xF3; frame[idx++] = 0xF2; frame[idx++] = 0xF1;
  int len_idx = idx; idx += 2; // Placeholder for length
  int target_count_idx = idx; idx++; 
  frame[idx++] = 0x01; // Alarm active

  for (int i = 0; i < MAX_TARGETS; i++) {
    if (chip->targets[i].active) {
      chip->targets[i].distance -= (chip->targets[i].speed_mps * 0.1f);

      // Car passes or goes out of range
      if (chip->targets[i].distance <= 1.0f || chip->targets[i].distance > 150.0f) {
        chip->targets[i].active = false;
        continue;
      }

      // 5-byte target block
      frame[idx++] = 0x80; // Angle
      frame[idx++] = (uint8_t)chip->targets[i].distance; //Distance in meters
      frame[idx++] = 0x01; // Approaching
      frame[idx++] = (uint8_t)(chip->targets[i].speed_mps * 3.6f); // Convert back to km/h
      frame[idx++] = (uint8_t)(255 - (chip->targets[i].distance * 2)); // SNR
      active_count++;
    }
  }

  if (active_count > 0) {
    // Finalize Frame
    frame[target_count_idx] = (uint8_t)active_count;
    uint16_t payload_len = (active_count * 5) + 2;
    frame[len_idx] = payload_len & 0xFF;
    frame[len_idx+1] = (payload_len >> 8) & 0xFF;
    
    frame[idx++] = 0xF8; frame[idx++] = 0xF7; frame[idx++] = 0xF6; frame[idx++] = 0xF5;
    uart_write(chip->uart, frame, idx);
  }
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
  chip->direction_filter = 1; // Default: Approaching Only
  chip->max_distance = 100;
  chip->min_speed = 5;
  chip->sensitivity = 5;
  chip->rx_idx = 0;

  // Ensure all potential car slots start as 'inactive'
  for (int i = 0; i < MAX_TARGETS; i++) {
    chip->targets[i].active = false;
    chip->targets[i].distance = 0;
    chip->targets[i].speed_mps = 0;
  }

  chip->next_wave_time_ns = get_sim_nanos() + 1000000000;
  // Start timer for radar simulation
  const timer_config_t timer_config = {
    .user_data = chip,
    .callback = on_timer,
  };
  timer_t timer_id = timer_init(&timer_config);
  timer_start(timer_id, 100000, true); 

  printf("[RADAR CHIP] Multi-Target Emulator Online (115200 baud)\n");
}
