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

extern void ws2812_set_pixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
extern void ws2812_refresh(void);
extern void ws2812_clear(void);

#endif /* INC_BOARD_WS2812_H_ */
