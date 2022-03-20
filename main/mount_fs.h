#ifndef MOUNT_FS_H
#define MOUNT_FS_H

#include "stdio.h"
#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_log.h"
#include "esp_littlefs.h"

#define LFS_MNT_TAG "LFS mount"

esp_err_t init_fs() {
    ESP_LOGI(LFS_MNT_TAG, "Initializing LittelFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "",
        .partition_label = "littlefs",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    // Use settings defined above to initialize and mount LittleFS filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(LFS_MNT_TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(LFS_MNT_TAG, "Failed to find LittleFS partition");
        }
        else
        {
            ESP_LOGE(LFS_MNT_TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(LFS_MNT_TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(LFS_MNT_TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret;
}

#endif // MOUNT_FS_H
