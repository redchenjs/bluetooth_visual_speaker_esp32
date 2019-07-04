/*
 * nvs.c
 *
 *  Created on: 2018-03-30 16:45
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "nvs_flash.h"

#define TAG "nvs"

void nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "nvs initialized.");
}
