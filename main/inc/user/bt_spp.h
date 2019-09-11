/*
 * bt_spp.h
 *
 *  Created on: 2019-07-03 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_BT_SPP_H_
#define INC_USER_BT_SPP_H_

#include "esp_spp_api.h"

extern uint32_t spp_conn_handle;
extern esp_bd_addr_t spp_remote_bda;

extern void bt_app_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

#endif /* INC_USER_BT_SPP_H_ */
