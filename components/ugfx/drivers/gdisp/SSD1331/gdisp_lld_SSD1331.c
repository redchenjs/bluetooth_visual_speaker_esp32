/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gfx.h"

#if GFX_USE_GDISP

#if defined(GDISP_SCREEN_HEIGHT) || defined(GDISP_SCREEN_HEIGHT)
	#if GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_DIRECT
		#warning "GDISP: This low level driver does not support setting a screen size. It is being ignored."
	#elif GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_MACRO
		COMPILER_WARNING("GDISP: This low level driver does not support setting a screen size. It is being ignored.")
	#endif
	#undef GDISP_SCREEN_WIDTH
	#undef GDISP_SCREEN_HEIGHT
#endif

#define GDISP_DRIVER_VMT			GDISPVMT_SSD1331
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

#include "board_SSD1331.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
	#define GDISP_SCREEN_HEIGHT		64
#endif
#ifndef GDISP_SCREEN_WIDTH
	#define GDISP_SCREEN_WIDTH		96
#endif
#ifndef GDISP_INITIAL_CONTRAST
	#define GDISP_INITIAL_CONTRAST	100
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
	#define GDISP_INITIAL_BACKLIGHT	100
#endif

#define GDISP_FLG_NEEDFLUSH			(GDISP_FLG_DRIVER<<0)

#include "SSD1331.h"

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

// Some common routines and macros
#define write_reg(g, reg, data)		{ write_cmd(g, reg); write_data(g, data); }

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

static const uint8_t init_data[] = {
	SSD1331_DISPLAY_OFF,
	SSD1331_START_LINE, 0x00,
	SSD1331_COM_OFFSET, 0x00,
	SSD1331_PIXELS_NORMAL,
	SSD1331_MULTIPLEX, 0x3F,
	SSD1331_RESET, SSD1331_RESET_OFF,
	SSD1331_POWER, SSD1331_POWER_ON,
	SSD1331_PHASE_PERIOD, 0x31,
	SSD1331_CLOCKS, 0xF0,
	SSD1331_PRECHARGE_A, 0x64,
	SSD1331_PRECHARGE_B, 0x78,
	SSD1331_PRECHARGE_C, 0x64,
	SSD1331_PRECHARGE_VOLTAGE, 0x3A,
	SSD1331_DESELECT_VOLTAGE, 0x3E,
	SSD1331_CONTRAST_A, 0x91,
	SSD1331_CONTRAST_B, 0x50,
	SSD1331_CONTRAST_C, 0x7D,
	SSD1331_BRIGHTNESS, (GDISP_INITIAL_BACKLIGHT*10)/63,
	#if GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_RGB565
		SSD1331_MODE, SSD1331_MODE_16_BIT|SSD1331_MODE_COM_SPLIT|SSD1331_MODE_COLUMN_REVERSE|SSD1331_MODE_COM_REVERSE,
	#elif GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_BGR565
		SSD1331_MODE, SSD1331_MODE_16_BIT|SSD1331_MODE_COM_SPLIT|SSD1331_MODE_BGRSSD1331_MODE_COLUMN_REVERSE|SSD1331_MODE_COM_REVERSE,
	#elif GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_RGB332
		SSD1331_MODE, SSD1331_MODE_8_BIT|SSD1331_MODE_COM_SPLITSSD1331_MODE_COLUMN_REVERSE|SSD1331_MODE_COM_REVERSE,
	#elif GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_BGR332
		SSD1331_MODE, SSD1331_MODE_8_BIT|SSD1331_MODE_COM_SPLIT|SSD1331_MODE_BGRSSD1331_MODE_COLUMN_REVERSE|SSD1331_MODE_COM_REVERSE,
	#else
		#error "SSD1331: Invalid color format"
	#endif
	SSD1331_DRAW_MODE, SSD1331_DRAW_FILLRECT
};

static const uint8_t gray_scale_table[] = {
	SSD1331_GRAYSCALE,
    0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10,
	0x12, 0x15, 0x19, 0x1D, 0x21, 0x25, 0x2A, 0x30,
	0x36, 0x3C, 0x42, 0x48, 0x50, 0x58, 0x60, 0x68,
	0x70, 0x78, 0x82, 0x8C, 0x96, 0xA0, 0xAA, 0xB4
};

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
	g->priv = gfxAlloc(GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH * 2);
	if (g->priv == NULL) {
		return FALSE;
	}

	for(int i=0; i < GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH * 2; i++) {
		*((uint8_t *)g->priv + i) = 0x00;
	}

	// Initialise the board interface
	init_board(g);

	// Hardware reset
	setpin_reset(g, TRUE);
	gfxSleepMilliseconds(20);
	setpin_reset(g, FALSE);
	gfxSleepMilliseconds(20);

	for(int i=0;i<sizeof(init_data);i++)
		write_cmd(g, init_data[i]);

	for(int i=0;i<sizeof(gray_scale_table);i++)
		write_cmd(g, gray_scale_table[i]);

	write_cmd(g, SSD1331_DISPLAY_ON);

	/* Initialise the GDISP structure */
	g->g.Width = GDISP_SCREEN_WIDTH;
	g->g.Height = GDISP_SCREEN_HEIGHT;
	g->g.Orientation = GDISP_ROTATE_0;
	g->g.Powermode = powerOn;
	g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
	g->g.Contrast = GDISP_INITIAL_CONTRAST;
	return TRUE;
}

#if GDISP_HARDWARE_FLUSH
	LLDSPEC void gdisp_lld_flush(GDisplay *g) {
		if (!(g->flags & GDISP_FLG_NEEDFLUSH)) {
			return;
		}
		refresh_gram(g, (uint8_t *)g->priv);
		g->flags &= ~GDISP_FLG_NEEDFLUSH;
	}
#endif

#if GDISP_HARDWARE_STREAM_WRITE
	LLDSPEC	void gdisp_lld_write_start(GDisplay *g) {
	}
	#if GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_RGB565 || GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_BGR565
		LLDSPEC	void gdisp_lld_write_color(GDisplay *g) {
			LLDCOLOR_TYPE c;
			c = gdispColor2Native(g->p.color);
			*((uint8_t *)g->priv + g->p.y * 192 + g->p.x * 2 + 0) = c >> 8;
			*((uint8_t *)g->priv + g->p.y * 192 + g->p.x * 2 + 1) = c;
		}
	#else
		LLDSPEC	void gdisp_lld_write_color(GDisplay *g) {
		}
	#endif
	LLDSPEC	void gdisp_lld_write_stop(GDisplay *g) {
		g->flags |= GDISP_FLG_NEEDFLUSH;
	}
#endif

#if GDISP_HARDWARE_FILLS
	LLDSPEC void gdisp_lld_fill_area(GDisplay *g) {
		LLDCOLOR_TYPE c;
		c = gdispColor2Native(g->p.color);
		for (int j=g->p.y; j<(g->p.y + g->p.cy); j++) {
			for (int i=g->p.x; i<(g->p.x + g->p.cx); i++) {
				*((uint8_t *)g->priv + j * 192 + i * 2 + 0) = c >> 8;
				*((uint8_t *)g->priv + j * 192 + i * 2 + 1) = c;
			}
		}
		g->flags |= GDISP_FLG_NEEDFLUSH;
	}
#endif

#if GDISP_HARDWARE_DRAWPIXEL
	LLDSPEC void gdisp_lld_draw_pixel(GDisplay *g) {
		LLDCOLOR_TYPE c;
		c = gdispColor2Native(g->p.color);
		*((uint8_t *)g->priv + g->p.y * 192 + g->p.x * 2 + 0) = c >> 8;
		*((uint8_t *)g->priv + g->p.y * 192 + g->p.x * 2 + 1) = c;
		g->flags |= GDISP_FLG_NEEDFLUSH;
	}
#endif

#if GDISP_HARDWARE_PIXELREAD
	LLDSPEC color_t gdisp_lld_get_pixel_color(GDisplay *g) {
		return (*((uint8_t *)g->priv + g->p.y * 192 + g->p.x * 2 + 0) << 8) |
			   (*((uint8_t *)g->priv + g->p.y * 192 + g->p.x * 2 + 1));
	}
#endif

#if GDISP_NEED_SCROLL && GDISP_HARDWARE_SCROLL
	LLDSPEC void gdisp_lld_vertical_scroll(GDisplay *g) {
	}
#endif

#if GDISP_NEED_CONTROL && GDISP_HARDWARE_CONTROL
	LLDSPEC void gdisp_lld_control(GDisplay *g) {
		switch(g->p.x) {
		case GDISP_CONTROL_POWER:
			if (g->g.Powermode == (powermode_t)g->p.ptr)
				return;
			switch((powermode_t)g->p.ptr) {
			case powerOff:
			case powerSleep:
			case powerDeepSleep:
				write_cmd(g, SSD1331_DISPLAY_OFF);
				break;
			case powerOn:
				write_cmd(g, SSD1331_DISPLAY_ON);
				break;
			default:
				return;
			}
			g->g.Powermode = (powermode_t)g->p.ptr;
			return;

		case GDISP_CONTROL_ORIENTATION:
			if (g->g.Orientation == (orientation_t)g->p.ptr)
				return;
			switch((orientation_t)g->p.ptr) {
			case GDISP_ROTATE_0:
				g->g.Height = GDISP_SCREEN_HEIGHT;
				g->g.Width = GDISP_SCREEN_WIDTH;
				break;
			case GDISP_ROTATE_90:
				g->g.Height = GDISP_SCREEN_WIDTH;
				g->g.Width = GDISP_SCREEN_HEIGHT;
				break;
			case GDISP_ROTATE_180:
				g->g.Height = GDISP_SCREEN_HEIGHT;
				g->g.Width = GDISP_SCREEN_WIDTH;
				break;
			case GDISP_ROTATE_270:
				g->g.Height = GDISP_SCREEN_WIDTH;
				g->g.Width = GDISP_SCREEN_HEIGHT;
				break;
			default:
				return;
			}
			g->g.Orientation = (orientation_t)g->p.ptr;
			return;

        case GDISP_CONTROL_BACKLIGHT:
            if ((unsigned)g->p.ptr > 100)
            	g->p.ptr = (void *)100;
            write_cmd(g, SSD1331_BRIGHTNESS);
            write_cmd(g, ((unsigned)g->p.ptr*10)/63);
            g->g.Backlight = (unsigned)g->p.ptr;
            return;

		case GDISP_CONTROL_CONTRAST:
			return;

        default:
            return;
		}
	}
#endif

#endif /* GFX_USE_GDISP */
