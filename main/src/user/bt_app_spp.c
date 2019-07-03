/*
 * bt_app_spp.c
 *
 *  Created on: 2019-07-03 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <time.h>
#include <string.h>
#include <sys/time.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "os/core.h"
#include "os/firmware.h"

#define BT_SPP_TAG "bt_spp"
#define BT_OTA_TAG "bt_ota"

#define SPP_SERVER_NAME "OTA"

static struct timeval time_new, time_old;
static long data_num = 0;

static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

static uint8_t ota_running = 0;
static long image_length = 0;

static const esp_partition_t *update_partition = NULL;
static esp_ota_handle_t update_handle = 0;

static void bt_spp_print_speed(void)
{
    float time_old_s = time_old.tv_sec + time_old.tv_usec / 1000000.0;
    float time_new_s = time_new.tv_sec + time_new.tv_usec / 1000000.0;
    float time_interval = time_new_s - time_old_s;
    float speed = data_num * 8 / time_interval / 1000.0;
    ESP_LOGI(BT_SPP_TAG, "speed(%fs ~ %fs): %f kbit/s" , time_old_s, time_new_s, speed);
    data_num = 0;
    time_old.tv_sec = time_new.tv_sec;
    time_old.tv_usec = time_new.tv_usec;
}

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
        ESP_LOGI(BT_SPP_TAG, "SPP connection state: disconnected");
        if (ota_running == 1) {
            esp_ota_end(update_handle);
            xEventGroupSetBits(user_event_group, KEY_SCAN_BIT);
            ota_running  = 0;
            image_length = 0;
        }
        break;
    case ESP_SPP_START_EVT:
        break;
    case ESP_SPP_CL_INIT_EVT:
        break;
    case ESP_SPP_DATA_IND_EVT:
        if (ota_running == 0) {
            if (strncmp("FW+RST\r\n", (const char *)param->data_ind.data, param->data_ind.len) == 0) {
                ESP_LOGI(BT_SPP_TAG, "GET command: FW+RST");

                esp_restart();
            } else if (strncmp("FW+VER?\r\n", (const char *)param->data_ind.data, param->data_ind.len) == 0) {
                ESP_LOGI(BT_SPP_TAG, "GET command: FW+VER?");

                uint8_t rsp_len = strlen(firmware_get_version()) + 2;
                uint8_t *rsp_str = malloc(rsp_len * sizeof(uint8_t));
                strncpy((char *)rsp_str, firmware_get_version(), rsp_len - 2);
                rsp_str[rsp_len - 2] = '\r';
                rsp_str[rsp_len - 1] = '\n';

                esp_spp_write(param->write.handle, rsp_len, rsp_str);
            } else if (strncmp("FW+UPD:", (const char *)param->data_ind.data, 7) == 0) {
                sscanf((const char *)param->data_ind.data, "FW+UPD:%ld\r\n", &image_length);
                ESP_LOGI(BT_SPP_TAG, "GET command: FW+UPD:%ld", image_length);

                if (image_length != 0) {
                    uint8_t rsp_str[] = "OK\r\n";
                    esp_spp_write(param->write.handle, sizeof(rsp_str), rsp_str);

                    xEventGroupClearBits(user_event_group, KEY_SCAN_BIT);

                    update_partition = esp_ota_get_next_update_partition(NULL);
                    ESP_LOGI(BT_OTA_TAG, "writing to partition subtype %d at offset 0x%x",
                                update_partition->subtype, update_partition->address);
                    assert(update_partition != NULL);

                    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(BT_OTA_TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        goto exit;
                    }

                    ota_running = 1;
                    data_num = 0;

                    gettimeofday(&time_old, NULL);
                } else {
                    uint8_t rsp_str[] = "ERROR\r\n";
                    esp_spp_write(param->write.handle, sizeof(rsp_str), rsp_str);
                }
            }
        } else {
            esp_err_t err = esp_ota_write(update_handle, (const void *)param->data_ind.data, param->data_ind.len);
            if (err != ESP_OK) {
                ESP_LOGE(BT_OTA_TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                goto exit;
            }
            data_num += param->data_ind.len;
            ESP_LOGD(BT_OTA_TAG, "have written image length %ld", data_num);

            if (data_num == image_length) {
                if (esp_ota_end(update_handle) != ESP_OK) {
                    ESP_LOGE(BT_OTA_TAG, "esp_ota_end failed");
                    goto exit;
                }
                esp_err_t err = esp_ota_set_boot_partition(update_partition);
                if (err != ESP_OK) {
                    ESP_LOGE(BT_OTA_TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
                    goto exit;
                }
                gettimeofday(&time_new, NULL);
                bt_spp_print_speed();
                uint8_t rsp_str[] = "DONE\r\n";
                esp_spp_write(param->write.handle, sizeof(rsp_str), rsp_str);
                xEventGroupSetBits(user_event_group, KEY_SCAN_BIT);
exit:
                ota_running  = 0;
                image_length = 0;
            }
        }
        break;
    case ESP_SPP_CONG_EVT:
        break;
    case ESP_SPP_WRITE_EVT:
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(BT_SPP_TAG, "SPP connection state: connected");
        break;
    default:
        break;
    }
}
