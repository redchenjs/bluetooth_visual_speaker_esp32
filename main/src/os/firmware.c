/*
 * firmware.c
 *
 *  Created on: 2018-04-05 19:09
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#define TAG "os_firmware"

const char *version = CONFIG_FIRMWARE_VERSION;

const char *os_firmware_get_version(void)
{
    return version;
}
