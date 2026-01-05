/**
 * @file keypad.c
 * @brief Keypad input handling implementation.
 *
 * Implements keypad initialization, sampling via I2C and GPIO, and debouncing logic.
 */

#include <stdint.h>

#include "config.h"
#include "keypad.h"
#include "driver/i2c.h"

#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define WRITE_BIT I2C_MASTER_WRITE  /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ    /*!< I2C master read */
#define ACK_CHECK_EN 0x1            /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0           /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                 /*!< I2C ack value */
#define NACK_VAL 0x1                /*!< I2C nack value */

static uint32_t i2c_frequency = I2C_MASTER_FREQUENCY;
static i2c_port_t i2c_port = I2C_PORT;

/**
 * @brief Initialize the I2C master driver.
 *
 * Configures the I2C parameters for the keypad communication.
 *
 * @return ESP_OK on success, error code otherwise.
 */
static esp_err_t i2c_master_driver_initialize()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = i2c_frequency
    };
    return i2c_param_config(i2c_port, &conf);
}

/**
 * @brief Read keypad data from I2C device.
 *
 * Performs an I2C read operation to get the keypad button states.
 *
 * @return Byte read from the I2C keypad device.
 */
static uint8_t i2c_keypad_read()
{
    int len = 1;
    uint8_t *data = malloc(len);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, I2C_ADDR_KEYPAD << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data + len - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    uint8_t val = data[0];
    free(data);

    return val;
}

/**
 * @brief Initialize the keypad subsystem.
 *
 * Sets up I2C driver and configures GPIO pins for additional keypad inputs.
 */
void keypad_init(void)
{
  i2c_master_driver_initialize();
  i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
 gpio_set_direction(KEYPAD_IO_L, GPIO_MODE_INPUT);
 gpio_set_direction(KEYPAD_IO_R, GPIO_MODE_INPUT);
 gpio_set_direction(KEYPAD_IO_MENU, GPIO_MODE_INPUT);
}

/**
 * @brief Sample the current keypad state.
 *
 * Reads I2C data and GPIO levels to determine which buttons are pressed.
 *
 * @return Bitmask of currently pressed buttons.
 */
uint16_t keypad_sample(void)
{
	uint16_t sample = 0;

	uint8_t i2c_data = i2c_keypad_read();

	if (((1<<0)&i2c_data) == 0) {
		sample |= KEYPAD_START;
	}

	if (((1<<1)&i2c_data) == 0) {
		sample |= KEYPAD_SELECT;
	}

	if (((1<<2)&i2c_data) == 0) {
		sample |= KEYPAD_UP;
	}

	if (((1<<3)&i2c_data) == 0) {
		sample |= KEYPAD_DOWN;
	}

	if (((1<<4)&i2c_data) == 0) {
		sample |= KEYPAD_LEFT;
	}

	if (((1<<5)&i2c_data) == 0) {
		sample |= KEYPAD_RIGHT;
	}

	if (((1<<6)&i2c_data) == 0) {
		sample |= KEYPAD_A;
	}

	if (((1<<7)&i2c_data) == 0) {
		sample |= KEYPAD_B;
	}

	if (!gpio_get_level(KEYPAD_IO_MENU)) {
		sample |= KEYPAD_MENU;
	}

	if (!gpio_get_level(KEYPAD_IO_L)) {
		sample |= KEYPAD_L;
	}

	if (!gpio_get_level(KEYPAD_IO_R)) {
		sample |= KEYPAD_R;
	}

	return sample;
}

/**
	* @brief Debounce keypad input and detect changes.
	*
	* Implements a debouncing algorithm to filter out noise and detect button state changes.
	*
	* @param sample Current keypad sample.
	* @param changes Pointer to store the bitmask of buttons that changed state.
	* @return Debounced keypad state.
	*/
uint16_t keypad_debounce(uint16_t sample, uint16_t *changes)
{
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
