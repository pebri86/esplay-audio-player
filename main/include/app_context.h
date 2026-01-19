/**
 * @file app_context.h
 * @brief Application context header file.
 *
 * Defines the application context structure and functions for initialization
 * and deinitialization.
 */

#pragma once

#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lvgl.h"

// Forward declaration for ESP-IDF types
typedef struct esp_lcd_panel_t *esp_lcd_panel_handle_t;

/**
 * @brief Application context structure.
 *
 * Holds all the global state for the application, including LVGL, playlist, UI,
 * and keypad data.
 */
typedef struct {
  // LVGL
  SemaphoreHandle_t lvgl_mutex;        /**< Mutex for LVGL operations. */
  esp_lcd_panel_handle_t panel_handle; /**< Handle to the LCD panel. */

  // Keypad
  uint16_t keypad_changes; /**< Bitmask of keypad state changes. */
} app_context_t;

/**
 * @brief Initialize the application context.
 *
 * Sets up the context with default values and initializes sub-components.
 *
 * @param ctx Pointer to the application context to initialize.
 */
void app_context_init(app_context_t *ctx);

/**
 * @brief Deinitialize the application context.
 *
 * Cleans up resources associated with the context.
 *
 * @param ctx Pointer to the application context to deinitialize.
 */
void app_context_deinit(app_context_t *ctx);
