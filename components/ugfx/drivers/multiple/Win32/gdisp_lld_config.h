/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_CONFIG_H
#define _GDISP_LLD_CONFIG_H

#if GFX_USE_GDISP

/*===========================================================================*/
/* Driver hardware support.                                                  */
/*===========================================================================*/

// Calling gdispGFlush() is optional for this driver but can be used by the
//	application to force a display update. eg after streaming.

#define GDISP_HARDWARE_FLUSH			TRUE
#define GDISP_HARDWARE_CONTROL			TRUE

//#define GDISP_WIN32_STREAMING_TEST
#ifdef GDISP_WIN32_STREAMING_TEST
	// These streaming routines are here only to debug the high level gdisp
	//	code for streaming controllers. They are slow, inefficient and have
	//	lots of debugging turned on.
	#define GDISP_HARDWARE_STREAM_WRITE		TRUE
	#define GDISP_HARDWARE_STREAM_READ		TRUE
	#define GDISP_HARDWARE_STREAM_POS		TRUE
#else
	// The proper way on the Win32. These routines are nice and fast.
	#define GDISP_HARDWARE_DRAWPIXEL		TRUE
	#define GDISP_HARDWARE_FILLS			TRUE
	#define GDISP_HARDWARE_PIXELREAD		TRUE
	#define GDISP_HARDWARE_SCROLL			TRUE

	// Bit-blits on Win32 are currently only supported for True-Color bit-depths greater than 8 bits
	// Note: At the time this file is included we have not calculated all our color
	//			definitions so we need to do this by hand.
	#if !defined(GDISP_PIXELFORMAT)
		#define GDISP_HARDWARE_BITFILLS			TRUE
	#elif (GDISP_PIXELFORMAT & 0x2000) && (((GDISP_PIXELFORMAT & 0x0F00)>>8)+((GDISP_PIXELFORMAT & 0x00F0)>>4)+((GDISP_PIXELFORMAT & 0x000F))) > 8
		#define GDISP_HARDWARE_BITFILLS			TRUE
	#endif
#endif

#define GDISP_LLD_PIXELFORMAT				GDISP_PIXELFORMAT_BGR888

// This function allows you to specify the parent window for any ugfx display windows created.
// Passing a NULL will reset window creation to creating top level windows.
// Note: In order to affect any static displays it must be called BEFORE gfxInit().
// Note: Creating a window under a parent causes the Mouse to be disabled by default (rather than enabled as for a top window)
void gfxEmulatorSetParentWindow(void *hwnd);

#if GINPUT_NEED_MOUSE
	// This function allows you to inject mouse events into the ugfx mouse driver
	void gfxEmulatorMouseInject(GDisplay *g, uint16_t buttons, coord_t x, coord_t y);

	// This function enables you to turn on/off normal mouse functions on a ugfx Win32 display window.
	void gfxEmulatorMouseEnable(GDisplay *g, bool_t enabled);

	// This function enables you to capture mouse events on a ugfx Win32 display window.
	// Passing NULL turns off the capture
	void gfxEmulatorMouseCapture(GDisplay *g, void (*capfn)(void * hWnd, GDisplay *g, uint16_t buttons, coord_t x, coord_t y));
#endif

#endif	/* GFX_USE_GDISP */

#endif	/* _GDISP_LLD_CONFIG_H */
