/*
 * bt_app_core.c
 *
 *  Created on: 2019-04-29 12:33
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "user/bt_app_core.h"

#define BT_APP_CORE_TAG "bt_app_core"

static xQueueHandle s_bt_app_task_queue = NULL;

static void bt_app_work_dispatched(bt_app_msg_t *msg)
{
    if (msg->cb) {
        msg->cb(msg->event, msg->param);
    }
}

static void bt_app_task(void *pvParameter)
{
    bt_app_msg_t msg = {0};

    while (1) {
        if (xQueueReceive(s_bt_app_task_queue, &msg, portMAX_DELAY) == pdTRUE) {
            switch (msg.sig) {
            case BT_APP_SIG_WORK_DISPATCH:
                bt_app_work_dispatched(&msg);
                break;
            default:
                break;
            }

            if (msg.param) {
                free(msg.param);
            }
        }
    }
}

static bool bt_app_send_msg(bt_app_msg_t *msg)
{
    if (msg == NULL) {
        return false;
    }

    if (xQueueSend(s_bt_app_task_queue, msg, 10 / portTICK_RATE_MS) != pdTRUE) {
        return false;
    }

    return true;
}

bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback)
{
    bt_app_msg_t msg = {0};

    msg.sig = BT_APP_SIG_WORK_DISPATCH;
    msg.event = event;
    msg.cb = p_cback;

    if (param_len == 0) {
        return bt_app_send_msg(&msg);
    } else if (p_params && param_len > 0) {
        if ((msg.param = malloc(param_len)) != NULL) {
            memcpy(msg.param, p_params, param_len);

            if (p_copy_cback) {
                p_copy_cback(&msg, msg.param, p_params);
            }

            return bt_app_send_msg(&msg);
        }
    }

    return false;
}

void bt_app_task_start_up(void)
{
    s_bt_app_task_queue = xQueueCreate(10, sizeof(bt_app_msg_t));

    xTaskCreatePinnedToCore(bt_app_task, "btAppT", 1920, NULL, configMAX_PRIORITIES - 3, NULL, 0);
}
