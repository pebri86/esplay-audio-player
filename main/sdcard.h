/**
 * @file sdcard.h
 * @brief SD card initialization and file listing header file.
 *
 * Defines the SD card mount point and functions for SD card operations.
 */

#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

/** SD card mount point path. */
#define MOUNT_POINT "/sdcard"

/**
 * @brief Initialize the SD card.
 *
 * Mounts the SD card filesystem.
 */
void sdcard_init();

/**
 * @brief List files on the SD card.
 *
 * Scans a directory on the SD card and lists files.
 *
 * @param dir_path Directory path to scan.
 * @param list Array to store file paths.
 * @param max_list Maximum number of files to list.
 * @return Number of files found.
 */
int list_files_on_sdcard(const char* dir_path, char list[][256], int max_list);