/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

// Define options for this driver
#define UC8173_REVERSEAXIS_Y		FALSE
#define UC8173_REVERSEAXIS_X		FALSE
#define UC8173_USE_OTP_LUT			FALSE		/* Use the LUT in the OTP - untested */
#define UC8173_DEFAULT_MODE			0			/* Which entry in the mode table to start with */
#define UC8173_CAN_READ				FALSE		/* Reading the controller chip is supported */
#define UC8173_VCOM_VOLTAGE			-2.80		/* Read this off the sticker on the back of the display or set UC8173_CAN_READ to have the chip read */
#define UC8171_BORDER				0			/* 0 = Hi-Z, 1 = Black, 2 = White */

// Define the waveform table
#include "UC8173_waveform_examples.h"
static UC8173Lut	UC8173_ModeTable[] = {
	// 32 bytes,				512 bytes,			128 bytes,	regal
	{ _lut_KWvcom_DC_A2_240ms,	_lut_kw_A2_240ms,	_lut_ft, 	FALSE },
	{ _lut_KWvcom_DC_A2_120ms,	_lut_kw_A2_120ms,	_lut_ft, 	FALSE },
	{ _lut_KWvcom_DC_GU,		_lut_kw_GU,			_lut_ft, 	TRUE  },
	{ _lut_KWvcom_GC,			_lut_kw_GC,			_lut_ft, 	FALSE }
	// Add extra lines for other waveforms
	};

static GFXINLINE bool_t init_board(GDisplay* g)
{
	(void) g;
	return TRUE;
}

static GFXINLINE void post_init_board(GDisplay* g)
{
	(void) g;
}

static GFXINLINE void setpin_reset(GDisplay* g, bool_t state)
{
	(void) g;
	(void) state;
}

static GFXINLINE bool_t getpin_busy(GDisplay* g)
{
	(void)g;
	return FALSE;
}

static GFXINLINE void acquire_bus(GDisplay* g)
{
	(void) g;
}

static GFXINLINE void release_bus(GDisplay* g)
{
	(void) g;
}

static GFXINLINE void write_cmd(GDisplay* g, uint8_t cmd)
{
	(void) g;
	(void) cmd;
}

static GFXINLINE void write_data(GDisplay* g, uint8_t data)
{
	(void) g;
	(void) data;
}

static GFXINLINE void write_data_burst(GDisplay* g, uint8_t* data, unsigned length)
{
	(void) g;
	(void) data;
	(void) length;
}

#if UC8173_CAN_READ
	static GFXINLINE uint8_t read_data(GDisplay* g)
	{
		(void)g;
		return 0;
	}
#endif

#endif /* _GDISP_LLD_BOARD_H */
