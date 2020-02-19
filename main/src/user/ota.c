/*
 * ota.c
 *
 *  Created on: 2020-02-12 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_spp_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"

#include "core/os.h"
#include "core/app.h"
#include "chip/i2s.h"
#include "user/vfx.h"
#include "user/ota.h"
#include "user/bt_av.h"
#include "user/bt_app.h"
#include "user/bt_spp.h"
#include "user/ble_app.h"
#include "user/ble_gatts.h"
#include "user/audio_input.h"
#include "user/audio_player.h"

#define OTA_TAG "ota"

#define ota_send_response(X) \
    esp_spp_write(spp_conn_handle, strlen(rsp_str[X]), (uint8_t *)rsp_str[X])

#define ota_send_data(X, N) \
    esp_spp_write(spp_conn_handle, N, (uint8_t *)X)

#define CMD_FMT_UPD "FW+UPD:%u"
#define CMD_FMT_RST "FW+RST!"
#define CMD_FMT_RAM "FW+RAM?"
#define CMD_FMT_VER "FW+VER?"

enum cmd_idx {
    CMD_IDX_UPD = 0x0,
    CMD_IDX_RST = 0x1,
    CMD_IDX_RAM = 0x2,
    CMD_IDX_VER = 0x3,
};

typedef struct {
    const char prefix;
    const char format[32];
} cmd_fmt_t;

static const cmd_fmt_t cmd_fmt[] = {
    { .prefix = 7, .format = CMD_FMT_UPD"\r\n" },   // Firmware Update
    { .prefix = 7, .format = CMD_FMT_RST"\r\n" },   // Chip Reset
    { .prefix = 7, .format = CMD_FMT_RAM"\r\n" },   // RAM Infomation
    { .prefix = 7, .format = CMD_FMT_VER"\r\n" },   // Firmware Version
};

enum rsp_idx {
    RSP_IDX_OK    = 0x0,
    RSP_IDX_FAIL  = 0x1,
    RSP_IDX_DONE  = 0x2,
    RSP_IDX_ERROR = 0x3,
};

static const char rsp_str[][32] = {
    "OK\r\n",           // OK
    "FAIL\r\n",         // Fail
    "DONE\r\n",         // Done
    "ERROR\r\n",        // Error
};

#ifdef CONFIG_ENABLE_VFX
    static vfx_config_t *vfx = NULL;
    static uint8_t vfx_prev_mode = 0;
#endif
#ifndef CONFIG_AUDIO_INPUT_NONE
    static uint8_t ain_prev_mode = 0;
#endif

static bool data_err = false;
static bool data_recv = false;
static uint32_t data_length = 0;

static uint8_t data_buff[990] = {0};
static RingbufHandle_t buff_handle = NULL;
static StaticRingbuffer_t buff_struct = {0};

static const esp_partition_t *update_partition = NULL;
static esp_ota_handle_t update_handle = 0;

static int ota_parse_command(esp_spp_cb_param_t *param)
{
    for (int i=0; i<sizeof(cmd_fmt)/sizeof(cmd_fmt_t); i++) {
        if (strncmp(cmd_fmt[i].format, (const char *)param->data_ind.data, cmd_fmt[i].prefix) == 0) {
            return i;
        }
    }
    return -1;
}

static void ota_write_task(void *pvParameter)
{
    uint8_t *data = NULL;
    uint32_t size = 0;
    esp_err_t err = ESP_OK;

    ESP_LOGI(OTA_TAG, "write started.");

    while (data_length > 0) {
        if (!data_recv || !update_handle) {
            ESP_LOGE(OTA_TAG, "write aborted.");

            goto write_fail;
        }

        if (data_length >= 990) {
            data = (uint8_t *)xRingbufferReceiveUpTo(buff_handle, &size, 10 / portTICK_RATE_MS, 990);
        } else {
            data = (uint8_t *)xRingbufferReceiveUpTo(buff_handle, &size, 10 / portTICK_RATE_MS, data_length);
        }

        if (data == NULL || size == 0) {
            continue;
        }

        err = esp_ota_write(update_handle, (const void *)data, size);
        if (err != ESP_OK) {
            ESP_LOGE(OTA_TAG, "write failed.");

            data_err = true;

            ota_send_response(RSP_IDX_FAIL);

            goto write_fail;
        }

        data_length -= size;

        if (data_length == 0) {
            esp_err_t err = esp_ota_end(update_handle);
            if (err != ESP_OK) {
                ESP_LOGE(OTA_TAG, "image data error.");

                data_err = true;

                ota_send_response(RSP_IDX_FAIL);

                goto write_fail;
            }

            err = esp_ota_set_boot_partition(update_partition);
            if (err != ESP_OK) {
                ESP_LOGE(OTA_TAG, "set boot partition failed.");

                data_err = true;

                ota_send_response(RSP_IDX_FAIL);

                goto write_fail;
            }
        }

        vRingbufferReturnItem(buff_handle, (void *)data);
    }

    ESP_LOGI(OTA_TAG, "write done.");

    ota_send_response(RSP_IDX_DONE);

write_fail:
    ota_end();

    vRingbufferDelete(buff_handle);
    buff_handle = NULL;

    vTaskDelete(NULL);
}

void ota_exec(esp_spp_cb_param_t *param)
{
    if (data_err) {
        return;
    }

    if (!data_recv) {
#ifdef CONFIG_ENABLE_VFX
        vfx = vfx_get_conf();
#endif

        int cmd_idx = ota_parse_command(param);

        switch (cmd_idx) {
            case CMD_IDX_UPD: {
                data_length = 0;
                sscanf((const char *)param->data_ind.data, CMD_FMT_UPD, &data_length);
                ESP_LOGI(OTA_TAG, "GET command: "CMD_FMT_UPD, data_length);

                EventBits_t uxBits = xEventGroupGetBits(user_event_group);
                if (data_length != 0 && !(uxBits & BT_OTA_LOCKED_BIT)
#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
                    && (uxBits & BLE_GATTS_IDLE_BIT)
#endif
                ) {
                    xEventGroupClearBits(user_event_group, KEY_SCAN_RUN_BIT);

#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
                    esp_ble_gap_stop_advertising();
#endif

#ifdef CONFIG_ENABLE_VFX
                    vfx_prev_mode = vfx->mode;
                    vfx->mode = VFX_MODE_IDX_OFF;
                    vfx_set_conf(vfx);
#endif
#ifndef CONFIG_AUDIO_INPUT_NONE
                    ain_prev_mode = audio_input_get_mode();
                    audio_input_set_mode(0);
#endif
#ifdef CONFIG_ENABLE_AUDIO_PROMPT
                    audio_player_set_mode(0);
                    xEventGroupWaitBits(
                        user_event_group,
                        AUDIO_PLAYER_IDLE_BIT,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY
                    );
#endif
                    i2s_output_deinit();

                    update_partition = esp_ota_get_next_update_partition(NULL);
                    if (update_partition != NULL) {
                        ESP_LOGI(OTA_TAG, "writing to partition subtype %d at offset 0x%x",
                                 update_partition->subtype, update_partition->address);
                    } else {
                        ESP_LOGE(OTA_TAG, "no ota partition to write");

                        ota_send_response(RSP_IDX_ERROR);

                        break;
                    }

                    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(OTA_TAG, "failed to start ota");

                        ota_send_response(RSP_IDX_ERROR);

                        break;
                    }

                    memset(&buff_struct, 0x00, sizeof(StaticRingbuffer_t));

                    buff_handle = xRingbufferCreateStatic(sizeof(data_buff), RINGBUF_TYPE_BYTEBUF, data_buff, &buff_struct);
                    if (!buff_handle) {
                        ota_send_response(RSP_IDX_ERROR);
                    } else {
                        data_recv = true;

                        ota_send_response(RSP_IDX_OK);

                        xTaskCreatePinnedToCore(ota_write_task, "otaWriteT", 4096, NULL, 9, NULL, 1);
                    }
                } else if (uxBits & BT_OTA_LOCKED_BIT
#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
                        || !(uxBits & BLE_GATTS_IDLE_BIT)
#endif
                ) {
                    ota_send_response(RSP_IDX_FAIL);
                } else {
                    ota_send_response(RSP_IDX_ERROR);
                }

                break;
            }
            case CMD_IDX_RST: {
                ESP_LOGI(OTA_TAG, "GET command: "CMD_FMT_RST);

                xEventGroupSetBits(user_event_group, BT_OTA_LOCKED_BIT);

                memset(&last_remote_bda, 0x00, sizeof(esp_bd_addr_t));
                app_setenv("LAST_REMOTE_BDA", &last_remote_bda, sizeof(esp_bd_addr_t));

#ifdef CONFIG_ENABLE_VFX
                vfx->mode = VFX_MODE_IDX_OFF;
                vfx_set_conf(vfx);
#endif
#ifndef CONFIG_AUDIO_INPUT_NONE
                audio_input_set_mode(0);
#endif

                EventBits_t uxBits = xEventGroupGetBits(user_event_group);
                if (!(uxBits & BT_A2DP_IDLE_BIT)) {
                    esp_a2d_sink_disconnect(a2d_remote_bda);
                }
#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
                if (!(uxBits & BLE_GATTS_IDLE_BIT)) {
                    esp_ble_gatts_close(gl_profile_tab[PROFILE_A_APP_ID].gatts_if,
                                        gl_profile_tab[PROFILE_A_APP_ID].conn_id);
                    esp_ble_gatts_close(gl_profile_tab[PROFILE_B_APP_ID].gatts_if,
                                        gl_profile_tab[PROFILE_B_APP_ID].conn_id);
                }
                os_power_restart_wait(BT_SPP_IDLE_BIT | BT_A2DP_IDLE_BIT | BLE_GATTS_IDLE_BIT);
#else
                os_power_restart_wait(BT_SPP_IDLE_BIT | BT_A2DP_IDLE_BIT);
#endif

                esp_spp_disconnect(param->write.handle);

                break;
            }
            case CMD_IDX_RAM: {
                ESP_LOGI(OTA_TAG, "GET command: "CMD_FMT_RAM);

                char str_buf[40] = {0};
                ESP_LOGI(OTA_TAG, "free memory: %u bytes", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
                snprintf(str_buf, sizeof(str_buf), "%u\r\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

                ota_send_data(str_buf, strlen(str_buf));

                break;
            }
            case CMD_IDX_VER: {
                ESP_LOGI(OTA_TAG, "GET command: "CMD_FMT_VER);

                char str_buf[40] = {0};
                ESP_LOGI(OTA_TAG, "app version: %s", app_get_version());
                snprintf(str_buf, sizeof(str_buf), "%s\r\n", app_get_version());

                ota_send_data(str_buf, strlen(str_buf));

                break;
            }
            default:
                ESP_LOGW(OTA_TAG, "unknown command.");

                ota_send_response(RSP_IDX_ERROR);

                break;
        }
    } else {
        if (buff_handle) {
            xRingbufferSend(buff_handle, (void *)param->data_ind.data, param->data_ind.len, portMAX_DELAY);
        }
    }
}

void ota_end(void)
{
    data_err = false;
    data_recv = false;

    if (update_handle) {
        esp_ota_end(update_handle);
        update_handle = 0;

        data_length = 0;

        i2s_output_init();
        audio_player_set_mode(1);
#ifndef CONFIG_AUDIO_INPUT_NONE
        audio_input_set_mode(ain_prev_mode);
#endif
#ifdef CONFIG_ENABLE_VFX
        vfx->mode = vfx_prev_mode;
        vfx_set_conf(vfx);
#endif

#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
        esp_ble_gap_start_advertising(&adv_params);
#endif

        xEventGroupSetBits(user_event_group, KEY_SCAN_RUN_BIT);
    }
}
