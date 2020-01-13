/*
 * cube0414.h
 *
 *  Created on: 2018-05-10 17:01
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_BOARD_CUBE0414_H_
#define INC_BOARD_CUBE0414_H_

#include <stdint.h>

#include "chip/spi.h"

#define CUBE0414_X 8
#define CUBE0414_Y 8
#define CUBE0414_Z 8

extern void cube0414_init_board(void);

extern void cube0414_setpin_dc(spi_transaction_t *);

extern void cube0414_write_cmd(uint8_t cmd);
extern void cube0414_write_data(uint8_t data);
extern void cube0414_write_buff(uint8_t *buff, uint32_t n);
extern void cube0414_refresh_gram(uint8_t *gram);

#endif /* INC_BOARD_CUBE0414_H_ */
