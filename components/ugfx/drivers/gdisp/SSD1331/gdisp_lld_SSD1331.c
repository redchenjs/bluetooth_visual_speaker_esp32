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
