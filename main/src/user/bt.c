/*
 * bt.c
 *
 *  Created on: 2018-03-09 13:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "user/bt.h"
#include "user/bt_av.h"
#include "user/gatts.h"

static xQueueHandle bt_task_queue = NULL;

#define TAG "bt"

static bool bt_send_msg(bt_app_msg_t *msg)
{
    if (msg == NULL) {
        return false;
    }

    if (xQueueSend(bt_task_queue, msg, 10 / portTICK_RATE_MS) != pdTRUE) {
        ESP_LOGE(TAG, "%s xQueue send failed", __func__);
        return false;
    }
    return true;
}

static void bt_work_dispatched(bt_app_msg_t *msg)
{
    if (msg->cb) {
        msg->cb(msg->event, msg->param);
    }
}

bool bt_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback)
{
    ESP_LOGD(TAG, "%s event 0x%x, param len %d", __func__, event, param_len);

    bt_app_msg_t msg;
    memset(&msg, 0, sizeof(bt_app_msg_t));

    msg.sig = BT_SPK_SIG_WORK_DISPATCH;
    msg.event = event;
    msg.cb = p_cback;

    if (param_len == 0) {
        return bt_send_msg(&msg);
    } else if (p_params && param_len > 0) {
        if ((msg.param = malloc(param_len)) != NULL) {
            memcpy(msg.param, p_params, param_len);
            /* check if caller has provided a copy callback to do the deep copy */
            if (p_copy_cback) {
                p_copy_cback(&msg, msg.param, p_params);
            }
            return bt_send_msg(&msg);
        }
    }

    return false;
}

void bt_task(void *pvParameter)
{
    gatts_init();
    bt_task_queue = xQueueCreate(10, sizeof(bt_app_msg_t));
    /* bluetooth device name, connection mode and profile set up */
    bt_work_dispatch(bt_av_hdl_stack_evt, BT_AV_EVT_STACK_UP, NULL, 0, NULL);
    bt_app_msg_t msg;
    while (1) {
        if (pdTRUE == xQueueReceive(bt_task_queue, &msg, portMAX_DELAY)) {
            ESP_LOGD(TAG, "%s, sig 0x%x, 0x%x", __func__, msg.sig, msg.event);
            switch (msg.sig) {
            case BT_SPK_SIG_WORK_DISPATCH:
                bt_work_dispatched(&msg);
                break;
            default:
                ESP_LOGW(TAG, "%s, unhandled sig: %d", __func__, msg.sig);
                break;
            }

            if (msg.param) {
                free(msg.param);
            }
        }
    }
}
