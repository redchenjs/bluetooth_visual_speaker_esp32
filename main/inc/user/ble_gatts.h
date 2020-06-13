/*
 * ble_gatts.h
 *
 *  Created on: 2018-05-12 22:31
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_BLE_GATTS_H_
#define INC_USER_BLE_GATTS_H_

#include "esp_gatts_api.h"

enum gatts_profile_idx {
    PROFILE_IDX_OTA,
    PROFILE_IDX_VFX,

    PROFILE_IDX_MAX,
};

typedef struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
} gatts_profile_inst_t;

extern gatts_profile_inst_t gatts_profile_tbl[];

extern void gatts_ota_send_notification(const char *data, uint32_t len);

extern void ble_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#endif /* INC_USER_BLE_GATTS_H_ */
