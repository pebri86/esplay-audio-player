/* Standard C Libraries */
#include <stdio.h>

/* RTOS and System */
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_sleep.h"

/* Display and Graphics */
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

/* Audio and External Components */
#include "audio_player.h"

/* Local Project Headers */
#include "config.h"
#include "audio.h"
#include "lcd.h"
#include "sdcard.h"
#include "keypad.h"
#include "mp3_metadata.h"
#include "ui_player.h"
#include "playlist.h"
#include "app_context.h"

static const char *TAG = TAG_MAIN;
static app_context_t app_ctx;

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
void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    lv_draw_sw_rgb565_swap(px_map, lv_area_get_size(area));
    esp_lcd_panel_draw_bitmap(app_ctx.panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
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
void lvgl_task(void *arg)
{
    while (1)
    {
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
 * This function resets the LCD panel, clears the RTC store register, and restarts the ESP32.
 */
static void app_shutdown(void)
{
    esp_lcd_panel_reset(app_ctx.panel_handle);
    REG_WRITE(RTC_CNTL_STORE0_REG, 0);
    esp_restart();
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
static void lv_keypad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t gamepad_state = keypad_debounce(keypad_sample(), &app_ctx.keypad_changes);

    // Default to released
    data->state = LV_INDEV_STATE_RELEASED;

    if (gamepad_state == KEYPAD_UP)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_UP;
    }
    else if (gamepad_state == KEYPAD_DOWN)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_DOWN;
    }
    else if (gamepad_state == KEYPAD_LEFT)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_LEFT;
    }
    else if (gamepad_state == KEYPAD_RIGHT)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_RIGHT;
    }
    else if (gamepad_state == KEYPAD_B)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_ESC;
    }
    else if (gamepad_state == KEYPAD_A)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_ENTER;
    }
    else if (gamepad_state == KEYPAD_MENU)
    {
        app_shutdown();
    }
    else if (gamepad_state == KEYPAD_L)
    {
        ui_decrease_volume(&app_ctx);
    }
    else if (gamepad_state == KEYPAD_R)
    {
        ui_increase_volume(&app_ctx);
    }
    // No 'else' needed here as data->state is initialized to RELEASED above
}

/**
 * @brief Button handler for UI controls.
 *
 * This function handles button presses from the UI, such as previous, play/pause, and next.
 * It updates the application context and controls the audio player accordingly.
 *
 * @param id The button ID (0: prev, 1: play/pause, 2: next).
 */
void btn_handler(int id)
{
    if (id == 0)
    { // prev
        app_ctx.skip_direction = -1;
        audio_player_stop();
    }
    else if (id == 1)
    { // play
        audio_player_state_t state = audio_player_get_state();
        if (state == AUDIO_PLAYER_STATE_PLAYING)
        {
            audio_player_pause();
        }
        else if (state == AUDIO_PLAYER_STATE_PAUSE)
        {
            audio_player_resume();
        }
    }
    else if (id == 2)
    { // next
        app_ctx.skip_direction = 1;
        audio_player_stop();
    }
}

/**
 * @brief Initialize LVGL graphics library.
 *
 * This function sets up LVGL with display buffers, flush callback, input device for keypad,
 * and creates the UI player interface. It also starts the LVGL task.
 *
 * @param panel_handle Handle to the LCD panel.
 */
static void init_lvgl(esp_lcd_panel_handle_t panel_handle)
{
    lv_init();
    app_ctx.lvgl_mutex = xSemaphoreCreateMutex();

    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

    static lv_color_t buf1[LCD_HEIGHT * LCD_WIDTH / 10];
    static lv_color_t buf2[LCD_HEIGHT * LCD_WIDTH / 10];
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_default(disp);

    // Create the input device object
    lv_indev_t *indev = lv_indev_create();
    if (!indev)
    {
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

    xTaskCreate(lvgl_task, "lvgl_task", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
}

/**
 * @brief Audio player mute callback function.
 *
 * This function handles mute/unmute requests from the audio player.
 * It pauses or resumes the audio output accordingly.
 *
 * @param setting Mute setting (0 for mute, non-zero for unmute).
 * @return ESP_OK on success.
 */
static esp_err_t _audio_player_mute_fn(AUDIO_PLAYER_MUTE_SETTING setting)
{
    ESP_LOGI(TAG, "mute setting: %s", setting == 0 ? "mute" : "unmute");
    (int)(setting == 0 ? audio_pause() : audio_resume());
    return ESP_OK;
}

/**
 * @brief Audio player write callback function.
 *
 * This function writes audio data to the I2S output.
 * It submits the audio buffer to the audio subsystem.
 *
 * @param audio_buffer Pointer to the audio buffer.
 * @param len Length of the buffer in bytes.
 * @param bytes_written Pointer to store the number of bytes written.
 * @param timeout_ms Timeout in milliseconds (unused).
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static esp_err_t _audio_player_write_fn(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    *bytes_written = 0;
    int n_frames = len / (2 * sizeof(short));
    int ret = audio_submit((short *)audio_buffer, n_frames);
    if (ret == 0)
    {
        *bytes_written = len;
        return ESP_OK;
    }
    else
    {
        return ESP_FAIL;
    }
}

/**
 * @brief Callback function to set the audio sample rate and configuration.
 *
 * This function is used by the audio player to configure the I2S clock settings.
 * It wraps the audio_set_sample_rate function and handles error mapping.
 *
 * @param rate Sample rate in Hz (e.g., 44100, 48000).
 * @param bits_cfg Bit configuration (e.g., 16, 24, 32 bits per sample).
 * @param ch I2S slot mode (mono or stereo).
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static esp_err_t _audio_player_clk_set_fn(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    int ret = audio_set_sample_rate(rate, bits_cfg, ch);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to set audio sample rate: rate=%lu, bits_cfg=%lu, ch=%d, error=%d",
                 (unsigned long)rate, (unsigned long)bits_cfg, (int)ch, ret);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief Play the current song from the playlist.
 *
 * This function retrieves the current song from the playlist, reads its metadata,
 * opens the file, and starts playing it using the audio player.
 * If playback fails, it attempts to play the next song.
 */
static void play_current_song(void)
{
    const char *current_file = playlist_get_current(&app_ctx.playlist);
    if (!current_file) {
        ESP_LOGE(TAG, "No current song to play");
        return;
    }

    mp3_metadata_t meta;
    if (mp3_read_metadata(current_file, &meta))
    {
        ESP_LOGI("META", "Title : %s", meta.title);
        ESP_LOGI("META", "Artist: %s", meta.artist);
        ESP_LOGI("META", "Album : %s", meta.album);
        ui_player_set_metadata(meta.title, meta.artist, &app_ctx);
    }

    FILE *fp = fopen(current_file, "rb");
    if (fp)
    {
        ESP_LOGI(TAG, "Playing '%s'", current_file);
        esp_err_t err = audio_player_play(fp);
        if (err == ESP_OK)
        {
            return; // success
        }
        else
        {
            ESP_LOGE(TAG, "Failed to play '%s', err %d", current_file, err);
            fclose(fp);
        }
    }
    else
    {
        ESP_LOGE(TAG, "unable to open filename '%s'", current_file);
    }
    // try next
    playlist_next(&app_ctx.playlist);
}

/**
 * @brief Audio player callback function.
 *
 * This function handles events from the audio player, such as idle, playing, and pause.
 * It manages playlist navigation and audio control based on the events.
 *
 * @param ctx Pointer to the audio player callback context.
 */
static void _audio_player_callback(audio_player_cb_ctx_t *ctx)
{
    ESP_LOGI(TAG, "ctx->audio_event = %d", ctx->audio_event);
    switch (ctx->audio_event)
    {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE:
    {
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_IDLE");
        if (app_ctx.skip_direction == -1)
        {
            playlist_prev(&app_ctx.playlist);
        }
        else
        {
            playlist_next(&app_ctx.playlist);
        }
        app_ctx.skip_direction = 0;
        play_current_song();
        break;
    }
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_PLAY");
        audio_resume();
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE:
        ESP_LOGI(TAG, "AUDIO_PLAYER_REQUEST_PAUSE");
        audio_pause();
        break;
    default:
        break;
    }
}

/**
 * @brief Main application entry point.
 *
 * This function initializes the application context, hardware components (SD card, LCD, keypad, audio),
 * sets up the UI with LVGL, loads the playlist, configures the audio player, and starts playback.
 */
void app_main(void)
{
    // Initialize application context
    app_context_init(&app_ctx);

    // Initialize hardware
    sdcard_init();
    lcd_init(&app_ctx.panel_handle);
    keypad_init();
    audio_init(DEFAULT_SAMPLE_RATE);

    // Initialize UI
    init_lvgl(app_ctx.panel_handle);

    // Load playlist
    if (!playlist_load_from_directory(&app_ctx.playlist, AUDIO_PATH)) {
        ESP_LOGE(TAG, "Failed to load playlist from %s", AUDIO_PATH);
    }

    // Initialize audio player
    audio_player_config_t config = {
        .mute_fn = _audio_player_mute_fn,
        .write_fn = _audio_player_write_fn,
        .clk_set_fn = _audio_player_clk_set_fn,
        .priority = AUDIO_PLAYER_PRIORITY,
        .coreID = AUDIO_PLAYER_CORE_ID
    };
    ESP_ERROR_CHECK(audio_player_new(config));
    ESP_ERROR_CHECK(audio_player_callback_register(_audio_player_callback, NULL));

    // Start playing
    play_current_song();

    // Main loop
    while (1)
    {
        vTaskDelay(100);
    }
}
