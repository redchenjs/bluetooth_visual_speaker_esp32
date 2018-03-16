/*
 * st7789.h
 *
 *  Created on: 2018-03-16 12:30
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_DRIVER_ST7789_H_
#define INC_DRIVER_ST7789_H_

#include <stdint.h>

#include "device/spi.h"

extern void st7789_init_board(void);
extern void st7789_setpin_dc(spi_transaction_t *);
extern void st7789_setpin_reset(uint8_t rst);

extern void st7789_write_cmd(uint8_t cmd);
extern void st7789_write_data(uint8_t data);
extern void st7789_refresh_gram(uint8_t *gram);

#endif /* INC_DRIVER_ST7789_H_ */
