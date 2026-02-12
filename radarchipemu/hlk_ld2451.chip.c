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
  
  // Configuration State
  bool config_enabled;
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
  if (chip->config_enabled) return;

  uint64_t now_ns = get_sim_nanos();

  if (!chip->car_active) {
    if (now_ns > chip->next_event_time_ns) {
      chip->car_active = true;
      chip->current_dist = rand() % 90 + 10;
      chip->speed_mps = (20.0 + (rand() % 80)) / 3.6; 
      printf("[RADAR CHIP] Car Spawned: %0.1f km/h\n", chip->speed_mps * 3.6);
    }
    return;
  }

  chip->current_dist -= (chip->speed_mps * 0.1);

  if (chip->current_dist <= 1.0) {
    chip->car_active = false;
    // Set next car spawn for 2-5 seconds later
    chip->next_event_time_ns = now_ns + (uint64_t)(2000 + (rand() % 3000)) * 1000000;
    printf("[RADAR CHIP] Car Passed.\n");
    return;
  }

  // Send radar data frame to host
  uint8_t frame[] = {
    0xF4, 0xF3, 0xF2, 0xF1, 0x07, 0x00, 0x01, 0x01, 0x80,
    (uint8_t)chip->current_dist, 0x01, (uint8_t)(chip->speed_mps * 3.6), 0xA0,
    0xF8, 0xF7, 0xF6, 0xF5
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

  chip->config_enabled = false; // Start in normal radar mode
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
