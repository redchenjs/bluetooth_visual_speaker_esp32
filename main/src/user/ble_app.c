/*
 * ble_app.c
 *
 *  Created on: 2019-07-04 22:50
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_gatts_api.h"
#include "esp_gap_bt_api.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"

#include "core/os.h"
#include "chip/bt.h"

#include "user/ble_gatts.h"

#define BLE_APP_TAG "ble_app"
#define BLE_GAP_TAG "ble_gap"

static uint8_t adv_data_raw[5 + sizeof(CONFIG_BT_NAME)] = {
    2,
    ESP_BT_EIR_TYPE_FLAGS,
    ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT,
    1 + sizeof(CONFIG_BT_NAME),
    ESP_BLE_AD_TYPE_NAME_CMPL
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
};

static void ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(BLE_GAP_TAG, "failed to start advertising");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(BLE_GAP_TAG, "failed to stop advertising");
        }
        break;
    default:
        break;
    }
}

esp_ble_adv_params_t *ble_app_get_adv_params(void)
{
    return &adv_params;
}

void ble_app_init(void)
{
    xEventGroupSetBits(user_event_group, BLE_GATTS_IDLE_BIT);

    esp_ble_gap_set_device_name(CONFIG_BT_NAME);
    esp_ble_gap_set_rand_addr(ble_get_mac_address());
    esp_ble_gap_register_callback(ble_gap_event_handler);

    strcpy((char *)adv_data_raw + 5, CONFIG_BT_NAME);
    esp_ble_gap_config_adv_data_raw(adv_data_raw, sizeof(adv_data_raw));

    esp_ble_gatts_register_callback(ble_gatts_event_handler);
    esp_ble_gatts_app_register(PROFILE_IDX_OTA);
    esp_ble_gatts_app_register(PROFILE_IDX_VFX);

    esp_ble_gatt_set_local_mtu(ESP_GATT_MAX_MTU_SIZE);

    ESP_LOGI(BLE_APP_TAG, "started.");
}
