/* Standard C Libraries */
#include <stdio.h>

/* RTOS and System */
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "soc/rtc_cntl_reg.h"

/* Display and Graphics */
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

/* Audio and External Components */
#include "audio_player.h"

/* Local Project Headers */
#include "app_context.h"
#include "audio.h"
#include "config.h"
#include "keypad.h"
#include "lcd.h"
#include "mp3_metadata.h"
#include "sdcard.h"
#include "ui_player.h"

static const char *TAG = TAG_MAIN;
app_context_t app_ctx;

/**
 * @brief LVGL display flush callback function.
 *
 * This function is called by LVGL to flush the display buffer to the LCD panel.
 * It swaps the RGB565 pixel data and draws the bitmap to the panel.
 *
 * @param disp Pointer to the LVGL display object.
 * @param area Pointer to the area structure defining the region to flush.
 * @param px_map Pointer to the pixel map data.
 */
void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  lv_draw_sw_rgb565_swap(px_map, lv_area_get_size(area));
  esp_lcd_panel_draw_bitmap(app_ctx.panel_handle, area->x1, area->y1,
                            area->x2 + 1, area->y2 + 1, px_map);
  lv_display_flush_ready(disp);
}

/**
 * @brief LVGL task function.
 *
 * This task runs in a loop to handle LVGL timers and events.
 * It increments the LVGL tick and calls the timer handler periodically.
 *
 * @param arg Unused parameter.
 */
void lvgl_task(void *arg) {
  while (1) {
    xSemaphoreTake(app_ctx.lvgl_mutex, portMAX_DELAY);
    lv_tick_inc(10);
    lv_timer_handler();
    xSemaphoreGive(app_ctx.lvgl_mutex);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/**
 * @brief Application shutdown function.
 *
 * This function resets the LCD panel, clears the RTC store register, and
 * restarts the ESP32.
 */
static void app_shutdown(void) {
  lcd_deinit(&app_ctx.panel_handle);
  REG_WRITE(RTC_CNTL_STORE0_REG, 0);
  esp_deep_sleep_start();
}

/**
 * @brief LVGL keypad read callback function.
 *
 * This function reads the keypad input and translates it to LVGL key events.
 * It handles various keypad buttons and maps them to LVGL keys or actions.
 *
 * @param indev Pointer to the LVGL input device.
 * @param data Pointer to the input device data structure to fill.
 */
static void lv_keypad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  uint16_t gamepad_state =
      keypad_debounce(keypad_sample(), &app_ctx.keypad_changes);

  // Default to released
  data->state = LV_INDEV_STATE_RELEASED;

  if (gamepad_state == KEYPAD_UP) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->key = LV_KEY_UP;
  } else if (gamepad_state == KEYPAD_DOWN) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->key = LV_KEY_DOWN;
  } else if (gamepad_state == KEYPAD_LEFT) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->key = LV_KEY_LEFT;
  } else if (gamepad_state == KEYPAD_RIGHT) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->key = LV_KEY_RIGHT;
  } else if (gamepad_state == KEYPAD_B) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->key = LV_KEY_ESC;
  } else if (gamepad_state == KEYPAD_A) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->key = LV_KEY_ENTER;
  } else if (gamepad_state == KEYPAD_MENU) {
    app_shutdown();
  } else if (gamepad_state == KEYPAD_L) {
    ui_decrease_volume(&app_ctx);
  } else if (gamepad_state == KEYPAD_R) {
    ui_increase_volume(&app_ctx);
  }
  // No 'else' needed here as data->state is initialized to RELEASED above
}

/**
 * @brief Button handler for UI controls.
 *
 * This function handles button presses from the UI, such as previous,
 * play/pause, and next. It updates the application context and controls the
 * audio player accordingly.
 *
 * @param id The button ID (0: prev, 1: play/pause, 2: next).
 */
void btn_handler(int id) {
  if (id == 0) { // prev
    player_send_cmd(PlayerCmdPrev);
  } else if (id == 1) { // play
    player_send_cmd(PlayerCmdPause);
  } else if (id == 2) { // next
    player_send_cmd(PlayerCmdNext);
  }
}

/**
 * @brief Initialize LVGL graphics library.
 *
 * This function sets up LVGL with display buffers, flush callback, input device
 * for keypad, and creates the UI player interface. It also starts the LVGL
 * task.
 *
 * @param panel_handle Handle to the LCD panel.
 */
static void init_lvgl(esp_lcd_panel_handle_t panel_handle) {
  lv_init();
  app_ctx.lvgl_mutex = xSemaphoreCreateMutex();

  lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

  static lv_color_t buf1[LCD_HEIGHT * LCD_WIDTH / 10];
  static lv_color_t buf2[LCD_HEIGHT * LCD_WIDTH / 10];
  lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_display_set_flush_cb(disp, lvgl_flush_cb);
  lv_display_set_default(disp);

  // Create the input device object
  lv_indev_t *indev = lv_indev_create();
  if (!indev) {
    ESP_LOGE(TAG, "Failed to create LVGL input device");
    return;
  }
  lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(indev, lv_keypad_read);

  lv_group_t *input_group = lv_group_create();
  lv_group_set_default(input_group);
  lv_indev_set_group(indev, input_group);

  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, UI_BG_COLOR, 0);
  ui_player_create(scr, btn_handler, input_group, &app_ctx);

  xTaskCreate(lvgl_task, "lvgl_task", LVGL_TASK_STACK_SIZE, NULL,
              LVGL_TASK_PRIORITY, NULL);
}

/**
 * @brief Main application entry point.
 *
 * This function initializes the application context, hardware components (SD
 * card, LCD, keypad, audio), sets up the UI with LVGL, loads the playlist,
 * configures the audio player, and starts playback.
 */
void app_main(void) {
  // Initialize application context
  app_context_init(&app_ctx);

  // Initialize hardware
  sdcard_init();
  lcd_init(&app_ctx.panel_handle);
  keypad_init();
  audio_init(DEFAULT_SAMPLE_RATE);

  // Initialize UI
  init_lvgl(app_ctx.panel_handle);

  player_start();
  // Main loop
  while (1) {
    vTaskDelay(100);
  }
}
