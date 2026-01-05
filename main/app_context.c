/**
 * @file app_context.c
 * @brief Application context implementation.
 *
 * Provides functions to initialize and deinitialize the application context.
 */

#include "app_context.h"
#include <string.h>

/**
 * @brief Initialize the application context.
 *
 * Zeroes the context structure, initializes the playlist, and creates the LVGL mutex.
 *
 * @param ctx Pointer to the application context to initialize.
 */
void app_context_init(app_context_t *ctx) {
    memset(ctx, 0, sizeof(app_context_t));
    playlist_init(&ctx->playlist);
    ctx->lvgl_mutex = xSemaphoreCreateMutex();
}

/**
 * @brief Deinitialize the application context.
 *
 * Deletes the LVGL mutex. Other resources are managed by their respective modules.
 *
 * @param ctx Pointer to the application context to deinitialize.
 */
void app_context_deinit(app_context_t *ctx) {
    if (ctx->lvgl_mutex) {
        vSemaphoreDelete(ctx->lvgl_mutex);
        ctx->lvgl_mutex = NULL;
    }
    // Note: Other resources should be cleaned up by their respective modules
    // LCD panel handle is managed by LCD module
    // Playlist data is static in this implementation
}