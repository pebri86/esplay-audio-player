/**
 * @file lcd.h
 * @brief LCD display initialization header file.
 *
 * Defines LCD dimensions and the initialization function.
 */

#include "esp_lcd_panel_io.h"

/** LCD display width in pixels. */
#define LCD_WIDTH 320
/** LCD display height in pixels. */
#define LCD_HEIGHT 240

/**
 * @brief Deinitialize the LCD display.
 *
 * Turns on the backlight and deletes the LCD panel.
 *
 * @param panel Pointer to the LCD panel handle to delete.
 */
void lcd_deinit(esp_lcd_panel_handle_t *panel);
/**
 * @brief Initialize the LCD display.
 *
 * Sets up the LCD panel and returns the panel handle.
 *
 * @param panel Pointer to store the LCD panel handle.
 */
void lcd_init(esp_lcd_panel_handle_t *panel);
