/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GINPUT_LLD_MOUSE_BOARD_H
#define _GINPUT_LLD_MOUSE_BOARD_H

// Resolution and Accuracy Settings
#define GMOUSE_EXC7200_PEN_CALIBRATE_ERROR		8
#define GMOUSE_EXC7200_PEN_CLICK_ERROR			6
#define GMOUSE_EXC7200_PEN_MOVE_ERROR			4
#define GMOUSE_EXC7200_FINGER_CALIBRATE_ERROR	14
#define GMOUSE_EXC7200_FINGER_CLICK_ERROR		18
#define GMOUSE_EXC7200_FINGER_MOVE_ERROR		14

// How much extra data to allocate at the end of the GMouse structure for the board's use
#define GMOUSE_EXC7200_BOARD_DATA_SIZE			0

static bool_t init_board(GMouse* m, unsigned driverinstance)
{
}

static GFXINLINE void aquire_bus(GMouse* m)
{
}

static GFXINLINE void release_bus(GMouse* m)
{
}

static void write_reg(GMouse* m, uint8_t reg, uint8_t val)
{
}

static uint8_t read_byte(GMouse* m, uint8_t reg)
{
}

static uint16_t read_word(GMouse* m, uint8_t reg)
{
}

#endif /* _GINPUT_LLD_MOUSE_BOARD_H */
