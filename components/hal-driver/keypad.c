/**
 * @file keypad.c
 * @brief Keypad input handling implementation (ESP-IDF 5.5.1).
 */

#include "keypad.h"
#include "hwconfig.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h" // Updated header
#include <stdint.h>

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t keypad_dev_handle;

/**
 * @brief Initialize the I2C master driver and keypad device.
 */
static esp_err_t i2c_master_driver_initialize() {
  // 1. Configure the I2C Bus
  i2c_master_bus_config_t bus_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_PORT,
      .scl_io_num = I2C_SCL,
      .sda_io_num = I2C_SDA,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

  // 2. Configure the Keypad Device on the bus
  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = I2C_ADDR_KEYPAD,
      .scl_speed_hz = I2C_MASTER_FREQUENCY,
  };
  return i2c_master_bus_add_device(bus_handle, &dev_config, &keypad_dev_handle);
}

/**
 * @brief Read keypad data from I2C device.
 */
static uint8_t i2c_keypad_read() {
  uint8_t data = 0;
  // New simplified receive API - handles start, addr, read, nack, and stop
  // internally
  esp_err_t ret = i2c_master_receive(keypad_dev_handle, &data, 1, -1);

  if (ret != ESP_OK) {
    // Handle error (e.g., return 0xFF to indicate no buttons pressed if active
    // low)
    return 0xFF;
  }

  return data;
}

/**
 * @brief Initialize the keypad subsystem.
 */
void keypad_init(void) {
  i2c_master_driver_initialize();

  // GPIO configuration using the newer gpio_config_t for best practice
  gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_INPUT,
      .pin_bit_mask = ((1ULL << KEYPAD_IO_L) | (1ULL << KEYPAD_IO_R) |
                       (1ULL << KEYPAD_IO_MENU)),
      .pull_down_en = 0,
      .pull_up_en = 1, // Assuming buttons pull to ground
  };
  gpio_config(&io_conf);
}

/**
 * @brief Sample the current keypad state.
 */
uint16_t keypad_sample(void) {
  uint16_t sample = 0;
  uint8_t i2c_data = i2c_keypad_read();

  // I2C Buttons (Active Low logic preserved)
  if (!(i2c_data & (1 << 0)))
    sample |= KEYPAD_START;
  if (!(i2c_data & (1 << 1)))
    sample |= KEYPAD_SELECT;
  if (!(i2c_data & (1 << 2)))
    sample |= KEYPAD_UP;
  if (!(i2c_data & (1 << 3)))
    sample |= KEYPAD_DOWN;
  if (!(i2c_data & (1 << 4)))
    sample |= KEYPAD_LEFT;
  if (!(i2c_data & (1 << 5)))
    sample |= KEYPAD_RIGHT;
  if (!(i2c_data & (1 << 6)))
    sample |= KEYPAD_A;
  if (!(i2c_data & (1 << 7)))
    sample |= KEYPAD_B;

  // GPIO Buttons
  if (!gpio_get_level(KEYPAD_IO_MENU))
    sample |= KEYPAD_MENU;
  if (!gpio_get_level(KEYPAD_IO_L))
    sample |= KEYPAD_L;
  if (!gpio_get_level(KEYPAD_IO_R))
    sample |= KEYPAD_R;

  return sample;
}

/**
 * @brief Debounce logic (remains logic-identical to your implementation)
 */
uint16_t keypad_debounce(uint16_t sample, uint16_t *changes) {
  static uint16_t state, cnt0, cnt1;
  uint16_t delta, toggle;

  delta = sample ^ state;
  cnt1 = (cnt1 ^ cnt0) & delta;
  cnt0 = ~cnt0 & delta;

  toggle = delta & ~(cnt0 | cnt1);
  state ^= toggle;
  if (changes) {
    *changes = toggle;
  }

  return state;
}
