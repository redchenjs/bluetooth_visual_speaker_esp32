/*
 * bt_av.h
 *
 *  Created on: 2018-03-09 13:56
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_TASKS_BT_AV_H_
#define INC_TASKS_BT_AV_H_

enum bt_av_evt_table {
    BT_AV_EVT_STACK_UP = 0,
};

#include <stdint.h>

#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

/* callback function for A2DP sink */
extern void bt_av_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
/* callback function for A2DP sink audio data stream */
extern void bt_av_a2d_data_cb(const uint8_t *data, uint32_t len);
/* callback function for AVRCP controller */
extern void bt_av_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);

/* stack event handler */
extern void bt_av_hdl_stack_evt(uint16_t event, void *p_param);
/* a2dp event handler */
extern void bt_av_hdl_a2d_evt(uint16_t event, void *p_param);
/* avrc event handler */
extern void bt_av_hdl_avrc_evt(uint16_t event, void *p_param);

#endif /* INC_TASKS_BT_AV_H_ */
