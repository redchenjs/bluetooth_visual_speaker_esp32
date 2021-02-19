/*
 * ble_gatts.c
 *
 *  Created on: 2018-05-12 22:31
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_gatts_api.h"
#include "esp_gap_bt_api.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"

#include "core/os.h"
#include "core/app.h"

#include "user/ota.h"
#include "user/vfx.h"
#include "user/ain.h"
#include "user/ble_app.h"
#include "user/ble_gatts.h"

#define BLE_GATTS_TAG "ble_gatts"

#define GATTS_OTA_TAG "gatts_ota"
#define GATTS_VFX_TAG "gatts_vfx"

#define GATTS_SRV_UUID_OTA   0xFF52
#define GATTS_CHAR_UUID_OTA  0x5201
#define GATTS_NUM_HANDLE_OTA 4

#define GATTS_SRV_UUID_VFX   0xFF53
#define GATTS_CHAR_UUID_VFX  0x5301
#define GATTS_NUM_HANDLE_VFX 4

static uint16_t desc_val_ota = 0x0000;
static uint16_t desc_val_vfx = 0x0000;

static const char *s_gatts_conn_state_str[] = {"disconnected", "connected"};

static void profile_ota_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void profile_vfx_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

gatts_profile_inst_t gatts_profile_tbl[PROFILE_IDX_MAX] = {
    [PROFILE_IDX_OTA] = { .gatts_cb = profile_ota_event_handler, .gatts_if = ESP_GATT_IF_NONE },
    [PROFILE_IDX_VFX] = { .gatts_cb = profile_vfx_event_handler, .gatts_if = ESP_GATT_IF_NONE }
};

static void profile_ota_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        gatts_profile_tbl[PROFILE_IDX_OTA].service_id.is_primary = true;
        gatts_profile_tbl[PROFILE_IDX_OTA].service_id.id.inst_id = 0x00;
        gatts_profile_tbl[PROFILE_IDX_OTA].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tbl[PROFILE_IDX_OTA].service_id.id.uuid.uuid.uuid16 = GATTS_SRV_UUID_OTA;

        esp_ble_gatts_create_service(gatts_if, &gatts_profile_tbl[PROFILE_IDX_OTA].service_id, GATTS_NUM_HANDLE_OTA);

        break;
    case ESP_GATTS_READ_EVT: {
        esp_gatt_rsp_t rsp = {0};

        if (param->read.handle == gatts_profile_tbl[PROFILE_IDX_OTA].descr_handle) {
            rsp.attr_value.len = 2;
            memcpy(rsp.attr_value.value, &desc_val_ota, sizeof(desc_val_ota));
        } else {
            rsp.attr_value.len = strlen(app_get_version()) < (ESP_GATT_DEF_BLE_MTU_SIZE - 3) ?
                                 strlen(app_get_version()) : (ESP_GATT_DEF_BLE_MTU_SIZE - 3);
            memcpy(rsp.attr_value.value, app_get_version(), rsp.attr_value.len);
        }

        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);

        break;
    }
    case ESP_GATTS_WRITE_EVT:
        if (!param->write.is_prep) {
            if (param->write.handle == gatts_profile_tbl[PROFILE_IDX_OTA].descr_handle) {
                desc_val_ota = param->write.value[1] << 8 | param->write.value[0];
            } else {
                ota_exec((const char *)param->write.value, param->write.len);
            }
        }

        if (param->write.need_rsp) {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        }

        break;
    case ESP_GATTS_CREATE_EVT:
        gatts_profile_tbl[PROFILE_IDX_OTA].service_handle = param->create.service_handle;
        gatts_profile_tbl[PROFILE_IDX_OTA].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tbl[PROFILE_IDX_OTA].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_OTA;

        esp_ble_gatts_start_service(gatts_profile_tbl[PROFILE_IDX_OTA].service_handle);

        esp_err_t add_char_ret = esp_ble_gatts_add_char(gatts_profile_tbl[PROFILE_IDX_OTA].service_handle,
                                                        &gatts_profile_tbl[PROFILE_IDX_OTA].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                                        NULL,
                                                        NULL);
        if (add_char_ret) {
            ESP_LOGE(GATTS_OTA_TAG, "failed to add char: %d", add_char_ret);
        }

        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        gatts_profile_tbl[PROFILE_IDX_OTA].char_handle = param->add_char.attr_handle;
        gatts_profile_tbl[PROFILE_IDX_OTA].descr_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tbl[PROFILE_IDX_OTA].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gatts_profile_tbl[PROFILE_IDX_OTA].service_handle,
                                                               &gatts_profile_tbl[PROFILE_IDX_OTA].descr_uuid,
                                                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                               NULL,
                                                               NULL);
        if (add_descr_ret) {
            ESP_LOGE(GATTS_OTA_TAG, "failed to add char descr: %d", add_descr_ret);
        }

        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gatts_profile_tbl[PROFILE_IDX_OTA].descr_handle = param->add_char_descr.attr_handle;
        break;
    case ESP_GATTS_CONNECT_EVT:
        xEventGroupClearBits(user_event_group, BLE_GATTS_IDLE_BIT);

        esp_ble_gap_stop_advertising();

        ESP_LOGI(GATTS_OTA_TAG, "connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_gatts_conn_state_str[1],
                 param->connect.remote_bda[0], param->connect.remote_bda[1],
                 param->connect.remote_bda[2], param->connect.remote_bda[3],
                 param->connect.remote_bda[4], param->connect.remote_bda[5]);

        gatts_profile_tbl[PROFILE_IDX_OTA].conn_id = param->connect.conn_id;

        break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_OTA_TAG, "connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_gatts_conn_state_str[0],
                 param->connect.remote_bda[0], param->connect.remote_bda[1],
                 param->connect.remote_bda[2], param->connect.remote_bda[3],
                 param->connect.remote_bda[4], param->connect.remote_bda[5]);

        ota_end();

        if (!(xEventGroupGetBits(user_event_group) & (OS_PWR_RESET_BIT | OS_PWR_SLEEP_BIT))) {
            esp_ble_gap_start_advertising(ble_app_get_adv_params());
        }

        xEventGroupSetBits(user_event_group, BLE_GATTS_IDLE_BIT);

        break;
    default:
        break;
    }
}

static void profile_vfx_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        gatts_profile_tbl[PROFILE_IDX_VFX].service_id.is_primary = true;
        gatts_profile_tbl[PROFILE_IDX_VFX].service_id.id.inst_id = 0x00;
        gatts_profile_tbl[PROFILE_IDX_VFX].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tbl[PROFILE_IDX_VFX].service_id.id.uuid.uuid.uuid16 = GATTS_SRV_UUID_VFX;

        esp_ble_gatts_create_service(gatts_if, &gatts_profile_tbl[PROFILE_IDX_VFX].service_id, GATTS_NUM_HANDLE_VFX);

        break;
    case ESP_GATTS_READ_EVT: {
        esp_gatt_rsp_t rsp = {0};

        if (param->read.handle == gatts_profile_tbl[PROFILE_IDX_VFX].descr_handle) {
            rsp.attr_value.len = 2;
            memcpy(rsp.attr_value.value, &desc_val_vfx, sizeof(desc_val_vfx));
        } else {
            rsp.attr_value.len = 8;
            /*
                BTT0: VFX Enabled
                BIT1: Cube Mode Enabled
                BIT2: Backlight Enabled
                BIT3: Audio Input Enabled
            */
            rsp.attr_value.value[0] = (
                0x00
#ifdef CONFIG_ENABLE_VFX
                | BIT0
    #if defined(CONFIG_VFX_OUTPUT_WS2812) || defined(CONFIG_VFX_OUTPUT_CUBE0414)
                | BIT1
    #endif
    #if defined(CONFIG_VFX_OUTPUT_ST7735) || defined(CONFIG_VFX_OUTPUT_ST7789)
                | BIT2
    #endif
    #ifndef CONFIG_AUDIO_INPUT_NONE
                | BIT3
    #endif
#endif
            );
#ifdef CONFIG_ENABLE_VFX
            vfx_config_t *vfx = vfx_get_conf();
    #ifndef CONFIG_AUDIO_INPUT_NONE
            ain_mode_t ain_mode = ain_get_mode();
    #endif
            rsp.attr_value.value[1] = vfx->mode;
            rsp.attr_value.value[2] = vfx->scale_factor >> 8;
            rsp.attr_value.value[3] = vfx->scale_factor & 0xFF;
            rsp.attr_value.value[4] = vfx->lightness >> 8;
            rsp.attr_value.value[5] = vfx->lightness & 0xFF;
            rsp.attr_value.value[6] = vfx->backlight;
    #ifndef CONFIG_AUDIO_INPUT_NONE
            rsp.attr_value.value[7] = ain_mode;
    #endif
#endif
        }

        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);

        break;
    }
    case ESP_GATTS_WRITE_EVT:
        if (!param->write.is_prep) {
            if (param->write.handle == gatts_profile_tbl[PROFILE_IDX_VFX].descr_handle) {
                desc_val_vfx = param->write.value[1] << 8 | param->write.value[0];
#ifdef CONFIG_ENABLE_VFX
            } else {
                vfx_config_t *vfx = vfx_get_conf();
    #ifndef CONFIG_AUDIO_INPUT_NONE
                ain_mode_t ain_mode = ain_get_mode();
    #endif
                switch (param->write.value[0]) {
                case 0xEF:
                    if (param->write.len == 1) {            // restore default configuration
                        vfx->mode = DEFAULT_VFX_MODE;
                        vfx->scale_factor = DEFAULT_VFX_SCALE_FACTOR;
                        vfx->lightness = DEFAULT_VFX_LIGHTNESS;
                        vfx->backlight = DEFAULT_VFX_BACKLIGHT;
                        vfx_set_conf(vfx);
                        app_setenv("VFX_INIT_CFG", vfx, sizeof(vfx_config_t));
    #ifndef CONFIG_AUDIO_INPUT_NONE
                        ain_mode = DEFAULT_AIN_MODE;
                        ain_set_mode(ain_mode);
                        app_setenv("AIN_INIT_CFG", &ain_mode, sizeof(ain_mode_t));
    #endif
                    } else if (param->write.len == 8) {     // apply new configuration
                        vfx->mode = param->write.value[1];
                        vfx->scale_factor = param->write.value[2] << 8 | param->write.value[3];
                        vfx->lightness = (param->write.value[4] << 8 | param->write.value[5]) % 0x0200;
                        vfx->backlight = param->write.value[6];
                        vfx_set_conf(vfx);
                        app_setenv("VFX_INIT_CFG", vfx, sizeof(vfx_config_t));
    #ifndef CONFIG_AUDIO_INPUT_NONE
                        ain_mode = param->write.value[7];
                        ain_set_mode(ain_mode);
                        app_setenv("AIN_INIT_CFG", &ain_mode, sizeof(ain_mode_t));
    #endif
                    } else {
                        ESP_LOGE(GATTS_VFX_TAG, "invalid command: 0x%02X", param->write.value[0]);
                    }
                    break;
                default:
                    ESP_LOGW(GATTS_VFX_TAG, "unknown command: 0x%02X", param->write.value[0]);
                    break;
                }
#endif
            }
        }

        if (param->write.need_rsp) {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        }

        break;
    case ESP_GATTS_CREATE_EVT:
        gatts_profile_tbl[PROFILE_IDX_VFX].service_handle = param->create.service_handle;
        gatts_profile_tbl[PROFILE_IDX_VFX].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tbl[PROFILE_IDX_VFX].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_VFX;

        esp_ble_gatts_start_service(gatts_profile_tbl[PROFILE_IDX_VFX].service_handle);

        esp_err_t add_char_ret = esp_ble_gatts_add_char(gatts_profile_tbl[PROFILE_IDX_VFX].service_handle,
                                                        &gatts_profile_tbl[PROFILE_IDX_VFX].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                                                        NULL,
                                                        NULL);
        if (add_char_ret) {
            ESP_LOGE(GATTS_VFX_TAG, "failed to add char: %d", add_char_ret);
        }

        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        gatts_profile_tbl[PROFILE_IDX_VFX].char_handle = param->add_char.attr_handle;
        gatts_profile_tbl[PROFILE_IDX_VFX].descr_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_tbl[PROFILE_IDX_VFX].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gatts_profile_tbl[PROFILE_IDX_VFX].service_handle,
                                                               &gatts_profile_tbl[PROFILE_IDX_VFX].descr_uuid,
                                                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                               NULL,
                                                               NULL);
        if (add_descr_ret) {
            ESP_LOGE(GATTS_VFX_TAG, "failed to add char descr: %d", add_descr_ret);
        }

        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gatts_profile_tbl[PROFILE_IDX_VFX].descr_handle = param->add_char_descr.attr_handle;
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(GATTS_VFX_TAG, "connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_gatts_conn_state_str[1],
                 param->connect.remote_bda[0], param->connect.remote_bda[1],
                 param->connect.remote_bda[2], param->connect.remote_bda[3],
                 param->connect.remote_bda[4], param->connect.remote_bda[5]);

        gatts_profile_tbl[PROFILE_IDX_VFX].conn_id = param->connect.conn_id;

        break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_VFX_TAG, "connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_gatts_conn_state_str[0],
                 param->connect.remote_bda[0], param->connect.remote_bda[1],
                 param->connect.remote_bda[2], param->connect.remote_bda[3],
                 param->connect.remote_bda[4], param->connect.remote_bda[5]);
        break;
    default:
        break;
    }
}

void ble_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gatts_profile_tbl[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGE(BLE_GATTS_TAG, "reg app failed, app_id: %04x, status: %d", param->reg.app_id, param->reg.status);
            return;
        }
    }

    for (int idx = 0; idx < PROFILE_IDX_MAX; idx++) {
        if (gatts_if == ESP_GATT_IF_NONE || gatts_if == gatts_profile_tbl[idx].gatts_if) {
            if (gatts_profile_tbl[idx].gatts_cb) {
                gatts_profile_tbl[idx].gatts_cb(event, gatts_if, param);
            }
        }
    }
}
