/**
 * @file sdcard.c
 * @brief SD card initialization and file listing implementation.
 *
 * Provides functions to mount the SD card and list MP3 files in directories.
 */

#include "sdcard.h"
#include "hwconfig.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

static const char *TAG = "sdcard storage";

/**
 * @brief Initialize the SD card.
 *
 * Mounts the SD card using SDMMC interface and FAT filesystem.
 */
void sdcard_init() {
  esp_err_t ret;
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = SDCARD_MAX_FILES,
      .allocation_unit_size = SDCARD_ALLOCATION_UNIT_SIZE};

  sdmmc_card_t *card;
  const char mount_point[] = MOUNT_POINT;
  ESP_LOGI(TAG, "Initializing SD card");
  ESP_LOGI(TAG, "Using SDMMC peripheral");
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = SDMMC_SLOT_WIDTH;
  slot_config.flags |= SDMMC_SLOT_FLAGS;

  ESP_LOGI(TAG, "Mounting filesystem");
  ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config,
                                &card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount filesystem.");
    } else {
      ESP_LOGE(TAG, "Failed to initialize the card (%s). ",
               esp_err_to_name(ret));
    }
    return;
  }
  ESP_LOGI(TAG, "Filesystem mounted on %s", MOUNT_POINT);
  // sdmmc_card_print_info(stdout, card);
}
