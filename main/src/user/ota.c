/*
 * ota.c
 *
 *  Created on: 2020-02-12 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_gap_bt_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"

#include "core/os.h"
#include "core/app.h"

#include "user/led.h"
#include "user/vfx.h"
#include "user/key.h"
#include "user/ain.h"
#include "user/bt_av.h"
#include "user/bt_app.h"
#include "user/ble_app.h"
#include "user/ble_gatts.h"

#define OTA_TAG "ota"

#define ota_send_response(X) \
    gatts_ota_send_notification((const char *)rsp_str[X], strlen(rsp_str[X]))

#define ota_send_data(X, N) \
    gatts_ota_send_notification((const char *)X, N)

#define RX_BUF_SIZE 512

#define CMD_FMT_UPD "FW+UPD:%u"
#define CMD_FMT_RST "FW+RST!"
#define CMD_FMT_RAM "FW+RAM?"
#define CMD_FMT_VER "FW+VER?"

enum cmd_idx {
    CMD_IDX_UPD = 0x0,
    CMD_IDX_RST = 0x1,
    CMD_IDX_RAM = 0x2,
    CMD_IDX_VER = 0x3
};

typedef struct {
    const char prefix;
    const char format[32];
} cmd_fmt_t;

static const cmd_fmt_t cmd_fmt[] = {
    { .prefix = 7, .format = CMD_FMT_UPD"\r\n" },
    { .prefix = 7, .format = CMD_FMT_RST"\r\n" },
    { .prefix = 7, .format = CMD_FMT_RAM"\r\n" },
    { .prefix = 7, .format = CMD_FMT_VER"\r\n" }
};

enum rsp_idx {
    RSP_IDX_OK    = 0x0,
    RSP_IDX_FAIL  = 0x1,
    RSP_IDX_DONE  = 0x2,
    RSP_IDX_ERROR = 0x3
};

static const char rsp_str[][32] = {
    "OK\r\n",
    "FAIL\r\n",
    "DONE\r\n",
    "ERROR\r\n"
};

#ifdef CONFIG_ENABLE_VFX
    static vfx_config_t *vfx = NULL;
    static vfx_mode_t vfx_prev_mode = VFX_MODE_IDX_OFF;
#endif
#ifndef CONFIG_AUDIO_INPUT_NONE
    static ain_mode_t ain_prev_mode = AIN_MODE_IDX_OFF;
#endif

static bool data_err = false;
static bool data_recv = false;
static uint32_t data_length = 0;

static RingbufHandle_t ota_buff = NULL;

static esp_ota_handle_t update_handle = 0;
static const esp_partition_t *update_partition = NULL;

static int ota_parse_command(const char *data)
{
    for (int i = 0; i < sizeof(cmd_fmt) / sizeof(cmd_fmt_t); i++) {
        if (strncmp(cmd_fmt[i].format, data, cmd_fmt[i].prefix) == 0) {
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
        if (!data_recv) {
            ESP_LOGE(OTA_TAG, "write aborted.");

            goto write_fail;
        }

        if (data_length >= RX_BUF_SIZE) {
            data = (uint8_t *)xRingbufferReceiveUpTo(ota_buff, &size, 10 / portTICK_RATE_MS, RX_BUF_SIZE);
        } else {
            data = (uint8_t *)xRingbufferReceiveUpTo(ota_buff, &size, 10 / portTICK_RATE_MS, data_length);
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
            err = esp_ota_end(update_handle);
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

        vRingbufferReturnItem(ota_buff, (void *)data);
    }

    ESP_LOGI(OTA_TAG, "write done.");

    ota_send_response(RSP_IDX_DONE);

write_fail:
    vRingbufferDelete(ota_buff);
    ota_buff = NULL;

    data_recv = false;

    vTaskDelete(NULL);
}

void ota_exec(const char *data, uint32_t len)
{
    if (data_err) {
        return;
    }

    if (!data_recv) {
#ifdef CONFIG_ENABLE_VFX
        vfx = vfx_get_conf();
#endif

        if (len <= 2) {
            return;
        }

        int cmd_idx = ota_parse_command(data);

        switch (cmd_idx) {
            case CMD_IDX_UPD: {
                data_length = 0;
                sscanf(data, CMD_FMT_UPD, &data_length);
                ESP_LOGI(OTA_TAG, "GET command: "CMD_FMT_UPD, data_length);

                EventBits_t uxBits = xEventGroupGetBits(user_event_group);
                if (data_length == 0) {
                    ota_send_response(RSP_IDX_ERROR);
                } else if (!(uxBits & BT_A2DP_IDLE_BIT) || (uxBits & BLE_GATTS_LOCK_BIT)) {
                    ota_send_response(RSP_IDX_FAIL);
                } else {
                    if (!update_handle) {
                        esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

#ifdef CONFIG_ENABLE_SLEEP_KEY
                        key_set_scan_mode(KEY_SCAN_MODE_IDX_OFF);
#endif
#ifdef CONFIG_ENABLE_LED
                        led_set_mode(LED_MODE_IDX_PULSE_D1);
#endif
#ifdef CONFIG_ENABLE_VFX
                        vfx_prev_mode = vfx->mode;
                        vfx->mode = VFX_MODE_IDX_OFF;
                        vfx_set_conf(vfx);
#endif
#ifndef CONFIG_AUDIO_INPUT_NONE
                        ain_prev_mode = ain_get_mode();
                        ain_set_mode(AIN_MODE_IDX_OFF);
#endif
#ifdef CONFIG_ENABLE_AUDIO_PROMPT
                        xEventGroupWaitBits(
                            user_event_group,
                            AUDIO_PLAYER_IDLE_BIT,
                            pdFALSE,
                            pdFALSE,
                            portMAX_DELAY
                        );
#endif
                    }

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

                    ota_buff = xRingbufferCreate(RX_BUF_SIZE, RINGBUF_TYPE_BYTEBUF);
                    if (!ota_buff) {
                        ota_send_response(RSP_IDX_ERROR);
                    } else {
                        data_recv = true;

                        ota_send_response(RSP_IDX_OK);

                        xTaskCreatePinnedToCore(ota_write_task, "otaWriteT", 1920, NULL, 10, NULL, 1);
                    }
                }

                break;
            }
            case CMD_IDX_RST: {
                ESP_LOGI(OTA_TAG, "GET command: "CMD_FMT_RST);

                xEventGroupSetBits(user_event_group, BLE_GATTS_LOCK_BIT);

                memset(&last_remote_bda, 0x00, sizeof(esp_bd_addr_t));
                app_setenv("LAST_REMOTE_BDA", &last_remote_bda, sizeof(esp_bd_addr_t));

                if (!update_handle) {
#ifdef CONFIG_ENABLE_SLEEP_KEY
                    key_set_scan_mode(KEY_SCAN_MODE_IDX_OFF);
#endif
#ifdef CONFIG_ENABLE_VFX
                    vfx->mode = VFX_MODE_IDX_OFF;
                    vfx_set_conf(vfx);
#endif
#ifndef CONFIG_AUDIO_INPUT_NONE
                    ain_set_mode(AIN_MODE_IDX_OFF);
#endif
                }

                esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

                if (!(xEventGroupGetBits(user_event_group) & BT_A2DP_IDLE_BIT)) {
                    esp_a2d_sink_disconnect(a2d_remote_bda);
                }

                esp_ble_gatts_close(gatts_profile_tbl[PROFILE_IDX_OTA].gatts_if,
                                    gatts_profile_tbl[PROFILE_IDX_OTA].conn_id);

                os_pwr_reset_wait(
                    BT_A2DP_IDLE_BIT | BLE_GATTS_IDLE_BIT
#ifdef CONFIG_ENABLE_AUDIO_PROMPT
                    | AUDIO_PLAYER_IDLE_BIT
#endif
                );

                update_handle = 0;

                break;
            }
            case CMD_IDX_RAM: {
                ESP_LOGI(OTA_TAG, "GET command: "CMD_FMT_RAM);

                char rsp_str[40] = {0};
                ESP_LOGI(OTA_TAG, "free memory: %u bytes", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
                snprintf(rsp_str, sizeof(rsp_str), "%u\r\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

                ota_send_data(rsp_str, strlen(rsp_str));

                break;
            }
            case CMD_IDX_VER: {
                ESP_LOGI(OTA_TAG, "GET command: "CMD_FMT_VER);

                char rsp_str[40] = {0};
                ESP_LOGI(OTA_TAG, "app version: %s", app_get_version());
                snprintf(rsp_str, sizeof(rsp_str), "%s\r\n", app_get_version());

                ota_send_data(rsp_str, strlen(rsp_str));

                break;
            }
            default:
                ESP_LOGW(OTA_TAG, "unknown command.");

                ota_send_response(RSP_IDX_ERROR);

                break;
        }
    } else {
        if (ota_buff) {
            xRingbufferSend(ota_buff, (void *)data, len, portMAX_DELAY);
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

        EventBits_t uxBits = xEventGroupGetBits(user_event_group);
        if (!(uxBits & OS_PWR_RESET_BIT) && !(uxBits & OS_PWR_SLEEP_BIT)) {
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        }

#ifndef CONFIG_AUDIO_INPUT_NONE
        ain_set_mode(ain_prev_mode);
#endif
#ifdef CONFIG_ENABLE_VFX
        vfx->mode = vfx_prev_mode;
        vfx_set_conf(vfx);
#endif
#ifdef CONFIG_ENABLE_LED
        led_set_mode(LED_MODE_IDX_BLINK_M0);
#endif
#ifdef CONFIG_ENABLE_SLEEP_KEY
        key_set_scan_mode(KEY_SCAN_MODE_IDX_ON);
#endif
    }
}
