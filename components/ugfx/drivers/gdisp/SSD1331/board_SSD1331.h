/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "driver/ssd1331.h"

#define init_board(g)           ssd1331_init()
#define write_cmd(g, cmd)       ssd1331_write_byte(cmd, 0)
#define write_data(g, data)     ssd1331_write_byte(data, 1)
#define refresh_gram(g, gram)   ssd1331_refresh_gram(gram)

#endif /* _GDISP_LLD_BOARD_H */
