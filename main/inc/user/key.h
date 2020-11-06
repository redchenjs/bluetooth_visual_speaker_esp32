/*
 * key.h
 *
 *  Created on: 2018-05-31 14:07
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_KEY_H_
#define INC_USER_KEY_H_

typedef enum {
    KEY_SCAN_MODE_IDX_OFF = 0x00,
    KEY_SCAN_MODE_IDX_ON  = 0x01
} key_scan_mode_t;

extern void key_set_scan_mode(key_scan_mode_t idx);
extern key_scan_mode_t key_get_scan_mode(void);

extern void key_init(void);

#endif /* INC_USER_KEY_H_ */
