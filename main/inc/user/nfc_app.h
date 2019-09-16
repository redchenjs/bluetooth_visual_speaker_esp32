/*
 * nfc_app.h
 *
 *  Created on: 2018-02-13 21:50
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_NFC_APP_H_
#define INC_USER_NFC_APP_H_

#include <stdint.h>

extern void nfc_app_set_mode(uint8_t idx);

extern void nfc_app_init(void);
extern void nfc_app_deinit(void);

#endif /* INC_USER_NFC_APP_H_ */
