/*
 * firmware.c
 *
 *  Created on: 2018-04-05 19:09
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_ota_ops.h"

#define TAG "os_firmware"

const char *os_firmware_get_version(void)
{
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();

    return (const char *)app_desc->version;
}
