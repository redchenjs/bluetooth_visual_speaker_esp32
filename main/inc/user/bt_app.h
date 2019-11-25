/*
 * bt_app.h
 *
 *  Created on: 2018-03-09 13:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_BT_APP_H_
#define INC_USER_BT_APP_H_

#include "esp_bt_device.h"

extern esp_bd_addr_t last_remote_bda;

extern void bt_app_init(void);

#endif /* INC_USER_BT_APP_H_ */
