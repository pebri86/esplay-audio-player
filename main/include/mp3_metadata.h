/**
 * @file mp3_metadata.h
 * @brief MP3 metadata reading header file.
 *
 * Defines the MP3 metadata structure and function for reading metadata from MP3
 * files.
 */

#pragma once
#include "config.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief MP3 metadata structure.
 *
 * Contains title, artist, and album information extracted from MP3 files.
 */
typedef struct {
  char title[METADATA_TITLE_MAX];   /**< Song title. */
  char artist[METADATA_ARTIST_MAX]; /**< Artist name. */
  char album[METADATA_ALBUM_MAX];   /**< Album name. */
} mp3_metadata_t;

/**
 * @brief Read metadata from an MP3 file.
 *
 * Parses the ID3 tags in the MP3 file and fills the metadata structure.
 *
 * @param path Path to the MP3 file.
 * @param meta Pointer to the metadata structure to fill.
 * @return true on success, false on failure.
 */
bool mp3_read_metadata(const char *path, mp3_metadata_t *meta);

