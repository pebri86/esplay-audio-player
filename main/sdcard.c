/**
 * @file sdcard.c
 * @brief SD card initialization and file listing implementation.
 *
 * Provides functions to mount the SD card and list MP3 files in directories.
 */

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "config.h"
#include "sdcard.h"

static const char *TAG = "sdcard storage";

/**
 * @brief Initialize the SD card.
 *
 * Mounts the SD card using SDMMC interface and FAT filesystem.
 */
void sdcard_init()
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = SDCARD_MAX_FILES,
        .allocation_unit_size = SDCARD_ALLOCATION_UNIT_SIZE
    };

    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");
    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = SDMMC_SLOT_WIDTH;
    slot_config.flags |= SDMMC_SLOT_FLAGS;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). ", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted on %s", MOUNT_POINT);
    //sdmmc_card_print_info(stdout, card);
}

/**
 * @brief List MP3 files on the SD card.
 *
 * Scans the specified directory and collects paths of MP3 files.
 *
 * @param dir_path Directory to scan.
 * @param list Array to store file paths.
 * @param max_list Maximum number of files to list.
 * @return Number of MP3 files found.
 */
int list_files_on_sdcard(const char* dir_path, char list[][256], int max_list) {
    ESP_LOGI(TAG, "Listing directory: %s", dir_path);

    DIR *dir = opendir(dir_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory %s", dir_path);
        return 0;
    }

    struct dirent *entry;
    int index = 0;
    while ((entry = readdir(dir)) != NULL && index < max_list) {
        if (entry->d_type == DT_DIR) {
            ESP_LOGI(TAG, "Found directory: %s", entry->d_name);
            // Optional: Recursively list files in subdirectories
            // char entry_path[256];
            // sprintf(entry_path, "%s/%s", dir_path, entry->d_name);
            // list_files_on_sdcard(entry_path);
        } else if (strlen(entry->d_name) > 4 && (strcasecmp(entry->d_name + strlen(entry->d_name) - 4, ".mp3") == 0)) {
            ESP_LOGI(TAG, "mp3 found: %s/%s", dir_path, entry->d_name);
            size_t dir_len = strlen(dir_path);
            size_t name_len = strlen(entry->d_name);
            if (dir_len + 1 + name_len + 1 <= MAX_FILENAME_LENGTH && index < max_list) {
                char entry_path[MAX_FILENAME_LENGTH] = {0};
                strcpy(entry_path, dir_path);
                strcat(entry_path, "/");
                strcat(entry_path, entry->d_name);
                strcpy(list[index], entry_path);
                index++;
            } else {
                ESP_LOGW(TAG, "Path too long or list full for file: %s/%s", dir_path, entry->d_name);
            }
        }
    }

    closedir(dir);
    return index;
}