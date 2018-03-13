/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "driver/ssd1331.h"

#define init_board(g)           ssd1331_init_board()
#define setpin_reset(g, rst)    ssd1331_setpin_reset(rst)
#define write_cmd(g, cmd)       ssd1331_write_cmd(cmd)
#define write_data(g, data)     ssd1331_write_data(data)
#define refresh_gram(g, gram)   ssd1331_refresh_gram(gram)

#endif /* _GDISP_LLD_BOARD_H */
