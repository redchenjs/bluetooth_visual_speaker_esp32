/*
 * bt_av.h
 *
 *  Created on: 2019-04-29 12:31
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_BT_AV_H_
#define INC_USER_BT_AV_H_

#include <stdint.h>

#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

extern esp_bd_addr_t a2d_remote_bda;
extern unsigned int a2d_sample_rate;

extern void bt_a2d_data_handler(const uint8_t *data, uint32_t len);
extern void bt_a2d_event_handler(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
extern void bt_avrc_ct_event_handler(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
extern void bt_avrc_tg_event_handler(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);

#endif /* INC_USER_BT_AV_H_*/
