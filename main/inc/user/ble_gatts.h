/*
 * ble_gatts.h
 *
 *  Created on: 2018-05-12 22:31
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_BLE_GATTS_H_
#define INC_USER_BLE_GATTS_H_

#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"

extern void ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
extern void ble_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#endif /* INC_USER_BLE_GATTS_H_ */
