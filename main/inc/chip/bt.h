/*
 * bt.h
 *
 *  Created on: 2018-03-09 10:36
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_CHIP_BT_H_
#define INC_CHIP_BT_H_

#include <stdint.h>

extern char *bt_get_mac_string(void);
extern char *ble_get_mac_string(void);

extern uint8_t *bt_get_mac_address(void);
extern uint8_t *ble_get_mac_address(void);

extern void bt_init(void);

#endif /* INC_CHIP_BT_H_ */
