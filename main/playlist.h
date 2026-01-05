/**
 * @file playlist.h
 * @brief Playlist management header file.
 *
 * Defines the playlist manager structure and functions for handling audio playlists.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/**
 * @brief Playlist manager structure.
 *
 * Manages a list of audio files and the current playback index.
 */
typedef struct {
    char playlists[MAX_PLAYLISTS][MAX_FILENAME_LENGTH]; /**< Array of file paths. */
    int num_playlists; /**< Number of files in the playlist. */
    int current_index; /**< Index of the currently playing file. */
} playlist_manager_t;

/**
 * @brief Initialize the playlist manager.
 *
 * Sets up the playlist with default values.
 *
 * @param pl Pointer to the playlist manager.
 */
void playlist_init(playlist_manager_t *pl);

/**
 * @brief Load playlist from a directory.
 *
 * Scans the directory for audio files and populates the playlist.
 *
 * @param pl Pointer to the playlist manager.
 * @param dir_path Path to the directory containing audio files.
 * @return true on success, false on failure.
 */
bool playlist_load_from_directory(playlist_manager_t *pl, const char *dir_path);

/**
 * @brief Get the current file in the playlist.
 *
 * @param pl Pointer to the playlist manager.
 * @return Path to the current file, or NULL if none.
 */
const char *playlist_get_current(playlist_manager_t *pl);

/**
 * @brief Move to the next file in the playlist.
 *
 * Advances the current index, wrapping around if necessary.
 *
 * @param pl Pointer to the playlist manager.
 */
void playlist_next(playlist_manager_t *pl);

/**
 * @brief Move to the previous file in the playlist.
 *
 * Decrements the current index, wrapping around if necessary.
 *
 * @param pl Pointer to the playlist manager.
 */
void playlist_prev(playlist_manager_t *pl);

/**
 * @brief Get the number of files in the playlist.
 *
 * @param pl Pointer to the playlist manager.
 * @return Number of files.
 */
int playlist_get_count(playlist_manager_t *pl);

/**
 * @brief Get the current index in the playlist.
 *
 * @param pl Pointer to the playlist manager.
 * @return Current index.
 */
int playlist_get_current_index(playlist_manager_t *pl);