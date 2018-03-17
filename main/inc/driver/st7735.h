/*
 * st7735.h
 *
 *  Created on: 2018-03-16 16:15
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_DRIVER_ST7735_H_
#define INC_DRIVER_ST7735_H_

#include <stdint.h>

#include "device/spi.h"

extern void st7735_init_board(void);
extern void st7735_setpin_dc(spi_transaction_t *);
extern void st7735_setpin_reset(uint8_t rst);

extern void st7735_write_cmd(uint8_t cmd);
extern void st7735_write_data(uint8_t data);
extern void st7735_refresh_gram(uint8_t *gram);

#endif /* INC_DRIVER_ST7735_H_ */
