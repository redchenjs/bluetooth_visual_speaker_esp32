/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#define SCREEN_WIDTH            800
#define SCREEN_HEIGHT           480
#define FRAMEREADER_BASE        ALT_VIP_VFR_0_BASE

#if GDISP_NEED_CONTROL
	static void board_backlight(GDisplay* g, uint8_t percent)
	{
		(void) g;
		(void) percent;
	}

	static void board_contrast(GDisplay* g, uint8_t percent)
	{
		(void) g;
		(void) percent;
	}

	static void board_power(GDisplay* g, powermode_t pwr)
	{
		(void) g;
		(void) pwr;
	}
#endif
