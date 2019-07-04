/*
 * ble_app.h
 *
 *  Created on: 2019-07-04 22:50
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_BLE_APP_H_
#define INC_USER_BLE_APP_H_

#include "esp_gap_ble_api.h"

extern esp_ble_adv_params_t adv_params;

extern void ble_app_init(void);

#endif /* INC_USER_BLE_APP_H_ */
