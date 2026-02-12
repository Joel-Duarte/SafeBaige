#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TARGETS 5 

typedef struct {
  float distance;
  float speed_mps;
  float angle;      // 128 is center (0 degrees)
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
    uint16_t cmd = chip->rx_buffer[6] | (chip->rx_buffer[7] << 8);

    printf("[RADAR] Command Received: 0x%04X (Config State: %s)\n", 
            cmd, chip->config_enabled ? "OPEN" : "LOCKED");

    switch (cmd) {
        case 0x00FF: 
            chip->config_enabled = true;
            printf("[RADAR] Mode -> CONFIG ENABLED\n");
            break;

        case 0x00FE: 
            chip->config_enabled = false;
            printf("[RADAR] Mode -> CONFIG CLOSED\n");
            break;

        case 0x0002: 
            if (chip->config_enabled) {
                chip->max_distance = chip->rx_buffer[8];
                chip->direction_filter = chip->rx_buffer[9];
                chip->min_speed = chip->rx_buffer[10];
                printf("[RADAR] UPDATE -> Range: %dm | Dir: %d | MinSpd: %d\n", 
                        chip->max_distance, chip->direction_filter, chip->min_speed);
            }
            break;

        case 0x0003: 
            if (chip->config_enabled) {
                chip->sensitivity = chip->rx_buffer[8];
                printf("[RADAR] UPDATE -> Sensitivity: %d\n", chip->sensitivity);
            }
            break;
    }
}

static void on_uart_data(void *user_data, uint8_t byte) {
    chip_state_t *chip = (chip_state_t *)user_data;
    if (chip->rx_idx < 32) chip->rx_buffer[chip->rx_idx++] = byte;

    if (chip->rx_idx >= 4 && memcmp(&chip->rx_buffer[chip->rx_idx - 4], CFG_FOOTER, 4) == 0) {
        if (memcmp(chip->rx_buffer, CFG_HEADER, 4) == 0) {
            process_command(chip);
        }
        chip->rx_idx = 0; 
    }
}

static void on_timer(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->config_enabled) return;

  uint64_t now_ns = get_sim_nanos();
  int active_count = 0;

  if (now_ns > chip->next_wave_time_ns) {
    int num_to_spawn = 1 + (rand() % 3); 
    printf("[RADAR] TRAFFIC WAVE: Spawning %d cars...\n", num_to_spawn);

    for (int i = 0, spawned = 0; i < MAX_TARGETS && spawned < num_to_spawn; i++) {
      if (!chip->targets[i].active) {
        chip->targets[i].active = true;
        chip->targets[i].distance = (float)chip->max_distance + (spawned * 10.0f);
        float speed_kmh = (float)(35 + (rand() % 45)); // Random speed between35 and 80 km/h
        chip->targets[i].speed_mps = speed_kmh / 3.6f;
        chip->targets[i].angle = 128.0f; // Start directly behind (0 degrees)
        spawned++;
      }
    }
    chip->next_wave_time_ns = now_ns + (uint64_t)(4000 + (rand() % 4000)) * 1000000;
  }

  uint8_t frame[64]; 
  int idx = 0;
  frame[idx++] = 0xF4; frame[idx++] = 0xF3; frame[idx++] = 0xF2; frame[idx++] = 0xF1;
  int len_idx = idx; idx += 2; 
  int target_count_idx = idx; idx++; 
  frame[idx++] = 0x01; 

  for (int i = 0; i < MAX_TARGETS; i++) {
    if (chip->targets[i].active) {
      
      // --- BRAKING LOGIC ---
      // If car is within 30m, simulate driver awareness decelerating to 25 km/h
      if (chip->targets[i].distance < 30.0f) {
        float target_speed_mps = 25.0f / 3.6f; 
        if (chip->targets[i].speed_mps > target_speed_mps) {
            chip->targets[i].speed_mps -= (1.5f * 0.1f); // Decelerate at 1.5 m/s^2
        }
      }

      chip->targets[i].distance -= (chip->targets[i].speed_mps * 0.1f);

      // Passing logic: If a car is between 12m and 2m, simulate it veering out to the left (angle < 128)
      if (chip->targets[i].distance <= 12.0f && chip->targets[i].distance > 2.0f) {
            // Map distance 12.0 -> 6.0 to angle 128 -> 80
            // This makes the car "pull out" into the passing lane
            float veerFactor = (12.0f - chip->targets[i].distance) * 8.0f; 
            chip->targets[i].angle = 128.0f - veerFactor;
        } else if (chip->targets[i].distance > 12.0f) {
            chip->targets[i].angle = 128.0f; // Stay in line
        }

        chip->targets[i].distance -= (chip->targets[i].speed_mps * 0.1f);

      if (chip->targets[i].distance <= 0.5f) {
        chip->targets[i].active = false;
        continue;
      }

      // noise logic
      float jitter = ((rand() % 200) - 100) / 100.0f; // Random -1.0 to +1.0 meters
      float noisyDist = chip->targets[i].distance + jitter;


      frame[idx++] = (uint8_t)chip->targets[i].angle;
      frame[idx++] = (uint8_t)noisyDist; 
      frame[idx++] = 0x01; 
      frame[idx++] = (uint8_t)(chip->targets[i].speed_mps * 3.6f); 
      frame[idx++] = (uint8_t)(255 - (chip->targets[i].distance * 2)); 
      active_count++;
    }
  }

  if (active_count > 0) {
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
  chip->config_enabled = false;
  chip->direction_filter = 1;
  chip->max_distance = 100;
  chip->min_speed = 5;
  chip->sensitivity = 5;
  chip->rx_idx = 0;

  for (int i = 0; i < MAX_TARGETS; i++) chip->targets[i].active = false;
  chip->next_wave_time_ns = get_sim_nanos() + 1000000000;
  const timer_config_t timer_config = { .user_data = chip, .callback = on_timer };
  timer_t timer_id = timer_init(&timer_config);
  timer_start(timer_id, 100000, true); 
}
