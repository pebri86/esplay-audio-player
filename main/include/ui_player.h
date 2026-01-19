/**
 * @file ui_player.h
 * @brief UI player interface header file.
 *
 * Defines the UI components and functions for the audio player interface.
 */

#include "app_context.h"
#include "lvgl.h"

/** Callback function type for button presses. */
typedef void (*ui_player_btn_cb_t)(int btn_id);

/**
 * @brief Create the UI player interface.
 *
 * Sets up the LVGL UI elements for the audio player.
 *
 * @param parent Parent LVGL object.
 * @param cb Callback for button presses.
 * @param input Input group for navigation.
 * @param ctx Application context.
 */
void ui_player_create(lv_obj_t *parent, ui_player_btn_cb_t cb,
                      lv_group_t *input, app_context_t *ctx);

/**
 * @brief Set metadata in the UI.
 *
 * Updates the displayed title and artist.
 *
 * @param title Song title.
 * @param artist Artist name.
 * @param ctx Application context.
 */
void ui_player_set_metadata(const char *title, const char *artist,
                            app_context_t *ctx);

/**
 * @brief Increase volume via UI.
 *
 * Increases the audio volume and updates the UI.
 *
 * @param ctx Application context.
 */
void ui_increase_volume(app_context_t *ctx);

/**
 * @brief Decrease volume via UI.
 *
 * Decreases the audio volume and updates the UI.
 *
 * @param ctx Application context.
 */
void ui_decrease_volume(app_context_t *ctx);
