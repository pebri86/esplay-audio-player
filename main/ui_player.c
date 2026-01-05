/**
 * @file ui_player.c
 * @brief UI player interface implementation.
 *
 * Implements the LVGL-based user interface for the audio player, including controls and metadata display.
 */

#include <stdbool.h>
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "config.h"
#include "ui_player.h"
#include "audio.h"

static const char *TAG = "ui player";

/* Global refs for update */
static lv_obj_t *vol;
static lv_obj_t *vol_value;
static lv_obj_t *label_title;
static lv_obj_t *label_artist;

static app_context_t *app_ctx = NULL;

static ui_player_btn_cb_t btn_cb = NULL;
static bool play = true;

/**
 * @brief Event callback for UI elements.
 *
 * Handles key presses, clicks, and value changes for buttons and slider.
 *
 * @param e LVGL event structure.
 */
static void event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e); // This is the button in the list

    if (code == LV_EVENT_KEY)
    {
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_RIGHT || key == LV_KEY_ESC)
            lv_group_focus_next(lv_group_get_default());
        else if (key == LV_KEY_LEFT)
            lv_group_focus_prev(lv_group_get_default());
    }
    else if (code == LV_EVENT_CLICKED)
    {
        int id = (int)lv_event_get_user_data(e);
        if (id == 1)
        {
            lv_obj_t *label = lv_obj_get_child(obj, 0);
            if (play) 
                lv_label_set_text(label, LV_SYMBOL_PLAY);
            else
                lv_label_set_text(label, LV_SYMBOL_PAUSE);

            play = !play;
        }
        if (btn_cb)
            btn_cb(id);
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {
        int value = (int)lv_slider_get_value(obj);
        audio_volume_set(value);
        lv_label_set_text_fmt(vol_value, "%d", value);
    }
}

/**
 * @brief Increase the volume via UI.
 *
 * Increments the volume and updates the UI elements.
 *
 * @param ctx Application context.
 */
void ui_increase_volume(app_context_t *ctx)
{
    int volume = audio_volume_get() + 1;
    audio_volume_set(volume);
    lv_label_set_text_fmt(vol_value, "%d", volume);
    lv_slider_set_value(vol, volume, LV_ANIM_ON);
}

/**
 * @brief Decrease the volume via UI.
 *
 * Decrements the volume and updates the UI elements.
 *
 * @param ctx Application context.
 */
void ui_decrease_volume(app_context_t *ctx)
{
    int volume = audio_volume_get() - 1;
    audio_volume_set(volume);
    lv_label_set_text_fmt(vol_value, "%d", volume);
    lv_slider_set_value(vol, volume, LV_ANIM_ON);
}

/**
 * @brief Set the metadata displayed in the UI.
 *
 * Updates the title and artist labels with the provided metadata.
 *
 * @param title Song title.
 * @param artist Artist name.
 * @param ctx Application context.
 */
void ui_player_set_metadata(const char *title, const char *artist, app_context_t *ctx)
{
    xSemaphoreTake(ctx->lvgl_mutex, portMAX_DELAY);
    lv_label_set_text(label_title,
                       (title && *title) ? title : "Unknown Title");

    lv_label_set_text(label_artist,
                       (artist && *artist) ? artist : "Unknown Artist");
    xSemaphoreGive(ctx->lvgl_mutex);
}

/**
 * @brief Create the UI player interface.
 *
 * Builds the LVGL UI with header, cover, song info, controls, and volume slider.
 *
 * @param parent Parent LVGL object.
 * @param cb Callback for button presses.
 * @param input Input group for navigation.
 * @param ctx Application context.
 */
void ui_player_create(lv_obj_t *parent, ui_player_btn_cb_t cb, lv_group_t *input, app_context_t *ctx)
{
    app_ctx = ctx;
    btn_cb = cb;
    /* ================= ROOT ================= */
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, UI_ROOT_WIDTH, UI_ROOT_HEIGHT);
    lv_obj_set_style_bg_color(root, UI_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);

    /* ================= HEADER ================= */
    lv_obj_t *header = lv_label_create(root);
    lv_label_set_text(header, "ESP32 Music Player");
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, UI_HEADER_Y, UI_HEADER_Y);
    lv_obj_set_style_text_color(header, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);

    /* ================= COVER ================= */
    lv_obj_t *cover = lv_obj_create(root);
    lv_obj_set_size(cover, UI_COVER_SIZE, UI_COVER_SIZE);
    lv_obj_align(cover, LV_ALIGN_LEFT_MID, UI_COVER_X, UI_COVER_Y);
    lv_obj_set_style_radius(cover, 10, 0);
    lv_obj_set_style_bg_color(cover, UI_COVER_BG_COLOR, 0);
    lv_obj_set_style_border_width(cover, 0, 0);

    lv_obj_t *icon = lv_label_create(cover);
    lv_label_set_text(icon, LV_SYMBOL_AUDIO);
    lv_obj_center(icon);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(icon, UI_TEXT_COLOR, 0);

    /* ================= SONG INFO ================= */
    label_title = lv_label_create(root);
    lv_label_set_text(label_title, "Unknown Title");
    lv_obj_align(label_title, LV_ALIGN_LEFT_MID, UI_TITLE_X, UI_TITLE_Y);
    lv_label_set_long_mode(label_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label_title, 200);
    lv_obj_set_style_text_color(label_title, UI_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0);

    label_artist = lv_label_create(root);
    lv_label_set_text(label_artist, "Unknown Artist");
    lv_obj_align(label_artist, LV_ALIGN_LEFT_MID, UI_ARTIST_X, UI_ARTIST_Y);
    lv_obj_set_style_text_color(label_artist, UI_SECONDARY_TEXT_COLOR, 0);

    /* ================= CONTROLS ================= */
    const char *btns[] = {
        LV_SYMBOL_PREV,
        LV_SYMBOL_PAUSE,
        LV_SYMBOL_NEXT,
        NULL};

    for (int i = 0; btns[i]; i++)
    {
        lv_obj_t *btn = lv_btn_create(root);
        lv_obj_set_size(btn, UI_BUTTON_SIZE, UI_BUTTON_SIZE);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT,
                      UI_BUTTONS_Y + (i * UI_BUTTON_SPACING) + 10, UI_BUTTONS_Y);
        lv_obj_set_style_radius(btn, UI_BUTTON_RADIUS, 0);
        lv_obj_set_style_bg_color(btn, UI_BUTTON_BG_COLOR, 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btns[i]);
        lv_obj_center(lbl);
        lv_obj_set_style_text_color(lbl, UI_BUTTON_TEXT_COLOR, 0);
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_ALL, (void *)i);
        lv_group_add_obj(input, btn);
    }

    /* ================= VOLUME ================= */

    vol_value = lv_label_create(root);
    lv_label_set_text_fmt(vol_value, "%d", audio_volume_get());
    lv_obj_align(vol_value, LV_ALIGN_BOTTOM_RIGHT,
                  UI_VOLUME_VALUE_X, UI_VOLUME_VALUE_Y);
    lv_obj_set_style_text_color(vol_value, UI_TEXT_COLOR, 0);

    lv_obj_t *vol_icon = lv_label_create(root);
    lv_label_set_text(vol_icon, LV_SYMBOL_VOLUME_MAX);
    lv_obj_align(vol_icon, LV_ALIGN_BOTTOM_RIGHT,
                  UI_VOLUME_ICON_X, UI_VOLUME_ICON_Y);
    lv_obj_set_style_text_color(vol_icon, UI_TEXT_COLOR, 0);

    vol = lv_slider_create(root);
    lv_obj_set_size(vol, UI_VOLUME_SLIDER_WIDTH, UI_VOLUME_SLIDER_HEIGHT);
    lv_obj_align(vol, LV_ALIGN_BOTTOM_RIGHT,
                  UI_VOLUME_SLIDER_X, UI_VOLUME_SLIDER_Y);
    lv_slider_set_value(vol, AUDIO_VOLUME_DEFAULT, LV_ANIM_OFF);
    lv_obj_add_event_cb(vol, event_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(input, vol);
}
