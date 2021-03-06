/*
 * gdisp_lld_board.h
 *
 *  Created on: 2018-05-10 16:56
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "board/ws2812.h"

#define init_board(g)           ws2812_init_board()
#define refresh_gram(g, gram)   ws2812_refresh_gram(gram)

#endif /* _GDISP_LLD_BOARD_H */
