/*
 * nfc_emulator.h
 *
 *  Created on: 2019-09-16 16:21
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_NFC_EMULATOR_H_
#define INC_USER_NFC_EMULATOR_H_

#include "nfc/nfc.h"

extern struct nfc_emulator nfc_app_emulator;

extern void nfc_emulator_update_bt_addr(const char *addr);

#endif /* INC_USER_NFC_EMULATOR_H_ */
