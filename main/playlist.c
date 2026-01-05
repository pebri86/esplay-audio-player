/**
 * @file playlist.c
 * @brief Playlist management implementation.
 *
 * Provides functions to initialize, load, and navigate playlists of audio files.
 */

#include "playlist.h"
#include "sdcard.h"
#include <string.h>
#include <esp_log.h>

static const char *TAG = TAG_SDCARD;

/**
 * @brief Initialize the playlist manager.
 *
 * Zeroes the playlist structure.
 *
 * @param pl Pointer to the playlist manager.
 */
void playlist_init(playlist_manager_t *pl) {
    memset(pl, 0, sizeof(playlist_manager_t));
}

/**
 * @brief Load playlist from a directory.
 *
 * Lists files from the SD card directory and populates the playlist.
 *
 * @param pl Pointer to the playlist manager.
 * @param dir_path Path to the directory.
 * @return true if files were loaded, false otherwise.
 */
bool playlist_load_from_directory(playlist_manager_t *pl, const char *dir_path) {
    pl->num_playlists = list_files_on_sdcard(dir_path, pl->playlists, MAX_PLAYLISTS);
    pl->current_index = 0;
    ESP_LOGI(TAG, "Loaded %d playlists from %s", pl->num_playlists, dir_path);
    return pl->num_playlists > 0;
}

/**
 * @brief Get the current file path.
 *
 * @param pl Pointer to the playlist manager.
 * @return Current file path or NULL if empty.
 */
const char *playlist_get_current(playlist_manager_t *pl) {
    if (pl->num_playlists == 0) {
        return NULL;
    }
    return pl->playlists[pl->current_index];
}

/**
 * @brief Advance to the next file.
 *
 * Increments the index with wrap-around.
 *
 * @param pl Pointer to the playlist manager.
 */
void playlist_next(playlist_manager_t *pl) {
    if (pl->num_playlists == 0) {
        return;
    }
    pl->current_index = (pl->current_index + 1) % pl->num_playlists;
}

/**
 * @brief Go to the previous file.
 *
 * Decrements the index with wrap-around.
 *
 * @param pl Pointer to the playlist manager.
 */
void playlist_prev(playlist_manager_t *pl) {
    if (pl->num_playlists == 0) {
        return;
    }
    pl->current_index = (pl->current_index - 1 + pl->num_playlists) % pl->num_playlists;
}

/**
 * @brief Get the number of files in the playlist.
 *
 * @param pl Pointer to the playlist manager.
 * @return Number of files.
 */
int playlist_get_count(playlist_manager_t *pl) {
    return pl->num_playlists;
}

/**
 * @brief Get the current index.
 *
 * @param pl Pointer to the playlist manager.
 * @return Current index.
 */
int playlist_get_current_index(playlist_manager_t *pl) {
    return pl->current_index;
}