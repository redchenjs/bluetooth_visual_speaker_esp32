/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

//Optional
//#define SSD1322_USE_DMA

#ifndef SSD1322_USE_DMA
	#define SSD1322_USE_DMA			FALSE
#endif

static GFXINLINE void init_board(GDisplay *g) {
	(void) g;
}

static GFXINLINE void post_init_board(GDisplay *g) {
	(void) g;
}

static GFXINLINE void setpin_reset(GDisplay *g, bool_t state) {
	(void) g;
	(void) state;
}

static GFXINLINE void acquire_bus(GDisplay *g) {
	(void) g;
}

static GFXINLINE void release_bus(GDisplay *g) {
	(void) g;
}


static GFXINLINE void write_cmd(GDisplay *g, uint8_t cmd) {
	(void) g;
	(void) cmd;
}

static GFXINLINE void write_data(GDisplay *g, uint8_t data) {
	(void) g;
	(void) data;
}

#if SSD1322_USE_DMA
	static GFXINLINE void write_data_DMA(GDisplay *g, uint8_t* data) {
		(void) g;
		(void) data;
	}
#endif	// Use DMA

#endif /* _GDISP_LLD_BOARD_H */
