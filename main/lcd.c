/**
 * @file lcd.c
 * @brief LCD display initialization implementation.
 *
 * Initializes the ILI9341 LCD panel via SPI and configures the backlight.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_ili9341.h"

#include "lcd.h"

#define LCD_HOST SPI2_HOST
#define LCD_CS 5
#define LCD_DC 12
#define LCD_RST -1
#define LCD_BK_LIGHT 27

/**
 * @brief Initialize the LCD backlight GPIO.
 *
 * Configures the backlight pin as output and turns it on.
 */
static void backlight_init()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_BK_LIGHT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LCD_BK_LIGHT, 1);
}

/**
 * @brief Initialize the LCD display.
 *
 * Sets up SPI bus, panel IO, and ILI9341 panel configuration.
 * Also initializes the backlight.
 *
 * @param panel Pointer to store the LCD panel handle.
 */
void lcd_init(esp_lcd_panel_handle_t *panel)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = 23,
        .miso_io_num = -1,
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_DC,
        .cs_gpio_num = LCD_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(*panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(*panel, true));

    backlight_init();
}