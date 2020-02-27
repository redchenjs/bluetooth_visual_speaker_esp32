/*
 * bt_app.c
 *
 *  Created on: 2018-03-09 13:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "core/os.h"
#include "core/app.h"
#include "user/bt_av.h"
#include "user/bt_spp.h"
#include "user/bt_app.h"
#include "user/bt_app_core.h"

#define BT_APP_TAG "bt_app"
#define BT_GAP_TAG "bt_gap"

esp_bd_addr_t last_remote_bda = {0};

/* event for handler "bt_app_hdl_stack_up */
enum {
    BT_APP_EVT_STACK_UP = 0,
};

static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(BT_GAP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
        } else {
            ESP_LOGE(BT_GAP_TAG, "authentication failed, status: %d", param->auth_cmpl.stat);
        }
        break;
    }
    default:
        break;
    }
}

static void bt_app_hdl_stack_evt(uint16_t event, void *p_param)
{
    switch (event) {
    case BT_APP_EVT_STACK_UP: {
        /* set up device name */
        esp_bt_dev_set_device_name(CONFIG_BT_NAME);

        /* register GAP callback */
        esp_bt_gap_register_callback(bt_app_gap_cb);

#ifdef CONFIG_ENABLE_OTA_OVER_SPP
        esp_spp_register_callback(bt_app_spp_cb);
        esp_spp_init(ESP_SPP_MODE_CB);
#endif

        /* initialize AVRCP controller */
        esp_avrc_ct_init();
        esp_avrc_ct_register_callback(bt_app_avrc_ct_cb);
        /* initialize AVRCP target */
        esp_avrc_tg_init();
        esp_avrc_tg_register_callback(bt_app_avrc_tg_cb);

        esp_avrc_rn_evt_cap_mask_t evt_set = {0};
        esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
        esp_avrc_tg_set_rn_evt_cap(&evt_set);

        /* initialize A2DP sink */
        esp_a2d_register_callback(&bt_app_a2d_cb);
        esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
        esp_a2d_sink_init();

        /* set discoverable and connectable mode, wait to be connected */
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

        break;
    }
    default:
        ESP_LOGE(BT_APP_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}

void bt_app_init(void)
{
    size_t length = sizeof(esp_bd_addr_t);
    app_getenv("LAST_REMOTE_BDA", &last_remote_bda, &length);

    xEventGroupSetBits(user_event_group, BT_SPP_IDLE_BIT);
    xEventGroupSetBits(user_event_group, BT_A2DP_IDLE_BIT);

    /* create application task */
    bt_app_task_start_up();

    /* Bluetooth device name, connection mode and profile set up */
    bt_app_work_dispatch(bt_app_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);

    /*
     * Set default parameters for Legacy Pairing
     * Use fixed pin code
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pin_code;
    pin_code[0] = '1';
    pin_code[1] = '2';
    pin_code[2] = '3';
    pin_code[3] = '4';
    esp_bt_gap_set_pin(pin_type, 4, pin_code);

    ESP_LOGI(BT_APP_TAG, "started.");

    if (memcmp(last_remote_bda, "\x00\x00\x00\x00\x00\x00", 6) != 0) {
        // Reconnection delay
        vTaskDelay(1000 / portTICK_RATE_MS);
#ifdef CONFIG_ENABLE_AUDIO_PROMPT
        xEventGroupWaitBits(
            user_event_group,
            AUDIO_PLAYER_IDLE_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );
#endif
        EventBits_t uxBits = xEventGroupGetBits(user_event_group);
        if (uxBits & BT_A2DP_IDLE_BIT) {
            ESP_LOGI(BT_APP_TAG, "reconnect to [%02x:%02x:%02x:%02x:%02x:%02x]",
                     last_remote_bda[0], last_remote_bda[1], last_remote_bda[2],
                     last_remote_bda[3], last_remote_bda[4], last_remote_bda[5]);

            esp_a2d_sink_connect(last_remote_bda);
        } else {
            ESP_LOGW(BT_APP_TAG, "reconnecting aborted");
        }
    }
}
