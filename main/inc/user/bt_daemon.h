/*
 * bt_daemon.h
 *
 *  Created on: 2018-03-09 13:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_BT_DAEMON_H_
#define INC_USER_BT_DAEMON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

enum bt_daemon_sig_table {
    BT_SPK_SIG_WORK_DISPATCH = 0x01,
};

/* handler for the dispatched work */
typedef void (*bt_app_cb_t) (uint16_t event, void *param);

/* message to be sent */
typedef struct {
    uint16_t        sig;      /*!< signal to bt_app_task */
    uint16_t        event;    /*!< message event id */
    bt_app_cb_t     cb;       /*!< context switch callback */
    void            *param;   /*!< parameter area needs to be last */
} bt_app_msg_t;

/* parameter deep-copy function to be customized */
typedef void (*bt_app_copy_cb_t) (bt_app_msg_t *msg, void *p_dest, void *p_src);

/* work dispatcher for the application task */
extern bool bt_daemon_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback);
extern void bt_daemon(void *pvParameter);

#endif /* INC_USER_BT_DEAMON_H_ */
