/*
 * gdisp_lld_board.h
 *
 *  Created on: 2019-04-29 22:04
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "board/st7789.h"

#define init_board(g)           st7789_init_board()
#define set_backlight(g, val)   st7789_set_backlight(val)
#define setpin_reset(g, val)    st7789_setpin_reset(val)
#define write_cmd(g, cmd)       st7789_write_cmd(cmd)
#define write_data(g, data)     st7789_write_data(data)
#define write_buff(g, buff, n)  st7789_write_buff(buff, n)
#define refresh_gram(g, gram)   st7789_refresh_gram(gram)

#endif /* _GDISP_LLD_BOARD_H */
