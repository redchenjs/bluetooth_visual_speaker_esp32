/*
 * gdisp_lld_board.h
 *
 *  Created on: 2018-05-10 16:56
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "board/ws2812.h"

#define init_board(g)                           ws2812_init_board()
#define set_pixel(g, index, red, green, blue)   ws2812_set_pixel(index, red, green, blue)
#define refresh(g)                              ws2812_refresh()
#define clear(g)                                ws2812_clear()

#endif /* _GDISP_LLD_BOARD_H */
