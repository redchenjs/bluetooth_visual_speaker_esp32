/*
 * bt_app_core.h
 *
 *  Created on: 2019-04-29 12:29
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_BT_APP_CORE_H_
#define INC_USER_BT_APP_CORE_H_

#include <stdint.h>

#define BT_APP_SIG_WORK_DISPATCH (0x01)

/**
 * @brief     handler for the dispatched work
 */
typedef void (*bt_app_cb_t)(uint16_t event, void *param);

/* message to be sent */
typedef struct {
    uint16_t sig;       /*!< signal to bt_app_task */
    uint16_t event;     /*!< message event id */
    bt_app_cb_t cb;     /*!< context switch callback */
    void *param;        /*!< parameter area needs to be last */
} bt_app_msg_t;

/**
 * @brief     parameter deep-copy function to be customized
 */
typedef void (*bt_app_copy_cb_t)(bt_app_msg_t *msg, void *p_dest, void *p_src);

/**
 * @brief     work dispatcher for the application task
 */
bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback);

void bt_app_task_start_up(void);

#endif /* INC_USER_BT_APP_CORE_H_ */
