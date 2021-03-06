/*
 * ws2812.h
 *
 *  Created on: 2020-06-25 22:01
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_BOARD_WS2812_H_
#define INC_BOARD_WS2812_H_

#include <stdint.h>

#define WS2812_X 8
#define WS2812_Y 8
#define WS2812_Z CONFIG_LED_PANEL_CASCADE

extern void ws2812_init_board(void);

extern void ws2812_refresh_gram(uint8_t *gram);

#endif /* INC_BOARD_WS2812_H_ */
