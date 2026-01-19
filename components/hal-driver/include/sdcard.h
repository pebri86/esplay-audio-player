/**
 * @file sdcard.h
 * @brief SD card initialization and file listing header file.
 *
 * Defines the SD card mount point and functions for SD card operations.
 */

#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

/** SD card mount point path. */
#define MOUNT_POINT "/sdcard"

/**
 * @brief Initialize the SD card.
 *
 * Mounts the SD card filesystem.
 */
void sdcard_init();
