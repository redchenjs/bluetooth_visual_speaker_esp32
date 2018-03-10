/*
 * spiffs.c
 *
 *  Created on: 2018-02-22 16:36
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "spiffs-0";

void spiffs0_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 32,
      .format_if_mount_failed = true
    };
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "failed to find spiffs partition");
        } else {
            ESP_LOGE(TAG, "failed to initialize spiffs (%d)", ret);
        }
        return;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to get spiffs partition information");
    } else {
        ESP_LOGI(TAG, "partition size: total: %d, used: %d", total, used);
    }
}
