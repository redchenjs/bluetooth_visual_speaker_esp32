/*
 * board_CUBE0414.h
 *
 *  Created on: 2018-05-10 16:56
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "driver/cube0414.h"

#define init_board(g)           cube0414_init_board()
#define write_cmd(g, cmd)       cube0414_write_cmd(cmd)
#define write_data(g, data)     cube0414_write_data(data)
#define refresh_gram(g, gram)   cube0414_refresh_gram(gram)

#endif /* _GDISP_LLD_BOARD_H */
