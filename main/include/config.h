#pragma once

/* Application Configuration */

// LVGL Timer Period
#define LVGL_TIMER_PERIOD_MS 5

// Audio Configuration
#define DEFAULT_SAMPLE_RATE 44100
#define AUDIO_VOLUME_DEFAULT 20

// Paths
#define AUDIO_FILE_PATH "/sdcard/audio"

// LCD Configuration (assuming from context)
#define LCD_WIDTH 320
#define LCD_HEIGHT 240

// Task Priorities
#define LVGL_TASK_PRIORITY 1
#define AUDIO_PLAYER_PRIORITY 1
#define AUDIO_PLAYER_CORE_ID 1

// Task Stack Sizes
#define LVGL_TASK_STACK_SIZE 8192

// LVGL Colors
#define UI_BG_COLOR lv_color_hex(0x0A1A2F)
#define UI_TEXT_COLOR lv_color_white()
#define UI_SECONDARY_TEXT_COLOR lv_color_hex(0xB0C4DE)
#define UI_BUTTON_BG_COLOR lv_color_hex(0x102A44)
#define UI_BUTTON_TEXT_COLOR lv_color_hex(0x2FD2FF)
#define UI_COVER_BG_COLOR lv_color_hex(0x1E90FF)

// UI Dimensions
#define UI_ROOT_WIDTH 320
#define UI_ROOT_HEIGHT 240
#define UI_COVER_SIZE 80
#define UI_BUTTON_SIZE 42
#define UI_BUTTON_RADIUS 21
#define UI_VOLUME_SLIDER_WIDTH 80
#define UI_VOLUME_SLIDER_HEIGHT 6

// UI Positions
#define UI_HEADER_Y 6
#define UI_COVER_X 10
#define UI_COVER_Y -10
#define UI_TITLE_X 100
#define UI_TITLE_Y -30
#define UI_ARTIST_X 100
#define UI_ARTIST_Y -5
#define UI_BUTTONS_Y -10
#define UI_BUTTON_SPACING 52
#define UI_VOLUME_VALUE_X -100
#define UI_VOLUME_VALUE_Y -26
#define UI_VOLUME_ICON_X -100
#define UI_VOLUME_ICON_Y -12
#define UI_VOLUME_SLIDER_X -10
#define UI_VOLUME_SLIDER_Y -18

// Metadata
#define METADATA_TITLE_MAX 64
#define METADATA_ARTIST_MAX 64
#define METADATA_ALBUM_MAX 64

// Logging Tags
#define TAG_MAIN "esplay audio player"
#define TAG_UI "ui player"