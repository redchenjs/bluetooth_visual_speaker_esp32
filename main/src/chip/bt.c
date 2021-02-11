/*
 * bt.c
 *
 *  Created on: 2018-03-09 10:36
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_bt.h"
#include "esp_log.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#define TAG "bt"

static char bt_mac_string[18] = {0};
static char ble_mac_string[18] = {0};

static uint8_t bt_mac_address[6] = {0};
static uint8_t ble_mac_address[6] = {0};

char *bt_get_mac_string(void)
{
    return bt_mac_string;
}

char *ble_get_mac_string(void)
{
    return ble_mac_string;
}

uint8_t *bt_get_mac_address(void)
{
    return bt_mac_address;
}

uint8_t *ble_get_mac_address(void)
{
    return ble_mac_address;
}

void bt_init(void)
{
#ifndef CONFIG_ENABLE_BLE_CONTROL_IF
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
#endif

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BTDM));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    memcpy(bt_mac_address, esp_bt_dev_get_address(), sizeof(bt_mac_address));
    memcpy(ble_mac_address, esp_bt_dev_get_address(), sizeof(ble_mac_address));

    ble_mac_address[0] |= 0xC0;

    snprintf(bt_mac_string, sizeof(bt_mac_string), MACSTR, MAC2STR(bt_mac_address));
    snprintf(ble_mac_string, sizeof(ble_mac_string), MACSTR, MAC2STR(ble_mac_address));

    ESP_LOGI(TAG, "initialized, bt: 1, ble: %d",
#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
             1
#else
             0
#endif
    );
}
