/*
 * st7735.h
 *
 *  Created on: 2018-03-16 16:15
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_BOARD_ST7735_H_
#define INC_BOARD_ST7735_H_

#include <stdint.h>

#include "chip/spi.h"

#define ST7735_SCREEN_WIDTH  80
#define ST7735_SCREEN_HEIGHT 160

extern void st7735_init_board(void);

extern void st7735_set_backlight(uint8_t val);
extern void st7735_setpin_dc(spi_transaction_t *);
extern void st7735_setpin_reset(uint8_t val);

extern void st7735_write_cmd(uint8_t cmd);
extern void st7735_write_data(uint8_t data);
extern void st7735_refresh_gram(uint8_t *gram);

#endif /* INC_BOARD_ST7735_H_ */
