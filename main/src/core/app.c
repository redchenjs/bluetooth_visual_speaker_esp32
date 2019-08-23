/*
 * app.c
 *
 *  Created on: 2018-04-05 19:09
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_ota_ops.h"

#define TAG "app"

const char *app_get_version(void)
{
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();

    return (const char *)app_desc->version;
}

void app_print_version(void)
{
    ESP_LOGW(TAG, "current app version is %s", app_get_version());
}
