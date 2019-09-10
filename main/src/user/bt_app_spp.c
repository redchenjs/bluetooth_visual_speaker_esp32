/*
 * bt_app_spp.c
 *
 *  Created on: 2019-07-03 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_spp_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "core/os.h"
#include "core/app.h"
#include "chip/i2s.h"
#include "user/led.h"
#include "user/vfx.h"
#include "user/ble_app.h"
#include "user/audio_input.h"

#define BT_SPP_TAG "bt_spp"
#define BT_OTA_TAG "bt_ota"

#define SPP_SERVER_NAME "OTA"

static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

uint8_t audio_input_prev_mode = 0;
uint8_t vfx_prev_mode = 0;

static long image_length = 0;
static long data_recv = 0;

static const esp_partition_t *update_partition = NULL;
static esp_ota_handle_t update_handle = 0;

static const char fw_cmd[][24] = {
    "FW+RST\r\n",       // Reset Device
    "FW+VER?\r\n",      // Get Firmware Version
    "FW+UPD:%ld\r\n"    // Update Device Firmware
};

static const char rsp_str[][24] = {
    "OK\r\n",           // OK
    "DONE\r\n",         // Done
    "ERROR\r\n",        // Error
    "LOCKED\r\n",       // Locked
    "VER:%s\r\n",       // Version
};

static const char *s_spp_conn_state_str[] = {"disconnected", "connected"};

esp_bd_addr_t spp_remote_bda = {0};

void bt_app_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
    case ESP_SPP_INIT_EVT:
        esp_spp_start_srv(sec_mask, role_slave, 0, SPP_SERVER_NAME);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        break;
    case ESP_SPP_OPEN_EVT:
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(BT_SPP_TAG, "SPP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_spp_conn_state_str[0],
                 spp_remote_bda[0], spp_remote_bda[1], spp_remote_bda[2],
                 spp_remote_bda[3], spp_remote_bda[4], spp_remote_bda[5]);

        EventBits_t uxBits = xEventGroupGetBits(user_event_group);
        if (uxBits & BT_OTA_RESTART_BIT) {
            ESP_LOGW(BT_SPP_TAG, "restart pending...");

            vTaskDelay(2000 / portTICK_RATE_MS);

            ESP_LOGW(BT_SPP_TAG, "restart now");
            esp_restart();
        }

        memset(&spp_remote_bda, 0x00, sizeof(esp_bd_addr_t));

        if (update_handle) {
            esp_ota_end(update_handle);
            update_handle = 0;

            image_length = 0;

            i2s_output_init();
#ifndef CONFIG_AUDIO_INPUT_NONE
            audio_input_set_mode(audio_input_prev_mode);
#endif
#ifdef CONFIG_ENABLE_VFX
            vfx_set_mode(vfx_prev_mode);
#endif
        }

        led_set_mode(3);

        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
        esp_ble_gap_start_advertising(&adv_params);
#endif

        xEventGroupSetBits(user_event_group, KEY_SCAN_RUN_BIT);
        break;
    case ESP_SPP_START_EVT:
        break;
    case ESP_SPP_CL_INIT_EVT:
        break;
    case ESP_SPP_DATA_IND_EVT:
        if (update_handle == 0) {
            if (strncmp(fw_cmd[0], (const char *)param->data_ind.data, strlen(fw_cmd[0])) == 0) {
                ESP_LOGI(BT_SPP_TAG, "GET command: FW+RST");

                xEventGroupSetBits(user_event_group, BT_OTA_RESTART_BIT);

                esp_spp_disconnect(param->write.handle);
            } else if (strncmp(fw_cmd[1], (const char *)param->data_ind.data, strlen(fw_cmd[1])) == 0) {
                ESP_LOGI(BT_SPP_TAG, "GET command: FW+VER?");

                char str_buf[24] = {0};
                snprintf(str_buf, sizeof(str_buf), rsp_str[4], app_get_version());

                esp_spp_write(param->write.handle, strlen(str_buf), (uint8_t *)str_buf);
            } else if (strncmp(fw_cmd[2], (const char *)param->data_ind.data, 7) == 0) {
                sscanf((const char *)param->data_ind.data, fw_cmd[2], &image_length);
                ESP_LOGI(BT_SPP_TAG, "GET command: FW+UPD:%ld", image_length);

                EventBits_t uxBits = xEventGroupGetBits(user_event_group);
                if (image_length != 0 && !(uxBits & BT_OTA_LOCKED_BIT) && !(uxBits & BLE_OTA_LOCKED_BIT)) {
#ifdef CONFIG_ENABLE_VFX
                    vfx_prev_mode = vfx_get_mode();
                    vfx_set_mode(0);
#endif
#ifndef CONFIG_AUDIO_INPUT_NONE
                    audio_input_prev_mode = audio_input_get_mode();
                    audio_input_set_mode(0);
#endif
                    i2s_output_deinit();

                    update_partition = esp_ota_get_next_update_partition(NULL);
                    if (update_partition != NULL) {
                        ESP_LOGI(BT_OTA_TAG, "writing to partition subtype %d at offset 0x%x",
                                 update_partition->subtype, update_partition->address);
                    } else {
                        ESP_LOGE(BT_OTA_TAG, "no ota partition to write");
                        goto err0;
                    }

                    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(BT_OTA_TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        goto err0;
                    }

                    data_recv = 0;

                    esp_spp_write(param->write.handle, strlen(rsp_str[0]), (uint8_t *)rsp_str[0]);
                } else if (uxBits & BT_OTA_LOCKED_BIT || uxBits & BLE_OTA_LOCKED_BIT) {
                    esp_spp_write(param->write.handle, strlen(rsp_str[3]), (uint8_t *)rsp_str[3]);
                } else {
                    esp_spp_write(param->write.handle, strlen(rsp_str[2]), (uint8_t *)rsp_str[2]);
                }
            } else {
                esp_spp_write(param->write.handle, strlen(rsp_str[2]), (uint8_t *)rsp_str[2]);
            }
        } else {
            esp_err_t err = esp_ota_write(update_handle, (const void *)param->data_ind.data, param->data_ind.len);
            if (err != ESP_OK) {
                ESP_LOGE(BT_OTA_TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                goto err1;
            }

            data_recv += param->data_ind.len;
            ESP_LOGD(BT_OTA_TAG, "have written image length %ld", data_recv);

            if (data_recv == image_length) {
                esp_err_t err = esp_ota_end(update_handle);
                if (err != ESP_OK) {
                    ESP_LOGE(BT_OTA_TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
                    goto err0;
                }

                err = esp_ota_set_boot_partition(update_partition);
                if (err != ESP_OK) {
                    ESP_LOGE(BT_OTA_TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
                    goto err0;
                }

                update_handle = 0;

                esp_spp_write(param->write.handle, strlen(rsp_str[1]), (uint8_t *)rsp_str[1]);
            } else if (data_recv > image_length) {
err1:
                esp_ota_end(update_handle);
                update_handle = 0;
err0:
                esp_spp_write(param->write.handle, strlen(rsp_str[2]), (uint8_t *)rsp_str[2]);

                i2s_output_init();
#ifndef CONFIG_AUDIO_INPUT_NONE
                audio_input_set_mode(audio_input_prev_mode);
#endif
#ifdef CONFIG_ENABLE_VFX
                vfx_set_mode(vfx_prev_mode);
#endif
            }
        }
        break;
    case ESP_SPP_CONG_EVT:
        break;
    case ESP_SPP_WRITE_EVT:
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        xEventGroupClearBits(user_event_group, KEY_SCAN_RUN_BIT);

        esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
        esp_ble_gap_stop_advertising();
#endif

        memcpy(&spp_remote_bda, param->srv_open.rem_bda, sizeof(esp_bd_addr_t));

        ESP_LOGI(BT_SPP_TAG, "SPP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_spp_conn_state_str[1],
                 spp_remote_bda[0], spp_remote_bda[1], spp_remote_bda[2],
                 spp_remote_bda[3], spp_remote_bda[4], spp_remote_bda[5]);

        led_set_mode(7);
        break;
    default:
        break;
    }
}
