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

#include "user/bt_app.h"
#include "user/bt_app_av.h"
#include "user/bt_app_core.h"

#define BT_APP_TAG "bt_app"
#define BT_GAP_TAG "bt_gap"

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
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(BT_GAP_TAG, "please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(BT_GAP_TAG, "passkey: %d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(BT_GAP_TAG, "please enter passkey");
        break;
    default:
        ESP_LOGE(BT_GAP_TAG, "invalid GAP event: %d", event);
        break;
    }
    return;
}

static void bt_app_hdl_stack_evt(uint16_t event, void *p_param)
{
    switch (event) {
    case BT_APP_EVT_STACK_UP: {
        /* set up device name */
        esp_bt_dev_set_device_name(CONFIG_BT_NAME);

        /* register GAP callback */
        esp_bt_gap_register_callback(bt_app_gap_cb);

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
    /* create application task */
    bt_app_task_start_up();

    /* Bluetooth device name, connection mode and profile set up */
    bt_app_work_dispatch(bt_app_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);

    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

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
}
