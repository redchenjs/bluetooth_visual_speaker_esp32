/*
 * pn532.h
 *
 *  Created on: 2018-04-23 13:10
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_BOARD_PN532_H_
#define INC_BOARD_PN532_H_

#include <stdint.h>

extern void pn532_setpin_reset(uint8_t val);
extern void pn532_init(void);

#endif /* INC_BOARD_PN532_H_ */
