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

#define GDISP_HARDWARE_FLUSH			TRUE
#define GDISP_HARDWARE_DRAWPIXEL		TRUE
//#define GDISP_HARDWARE_PIXELREAD		TRUE - not implemented yet
#define GDISP_HARDWARE_CONTROL      	TRUE
#define GDISP_HARDWARE_FILLS        	FALSE

#define GDISP_LLD_PIXELFORMAT			GDISP_PIXELFORMAT_MONO
//#define GDISP_LLD_PIXELFORMAT			GDISP_PIXELFORMAT_GRAY4

#define GDISP_CONTROL_INVERT			(GDISP_CONTROL_LLD+0)
#define GDISP_CONTROL_SETMODE			(GDISP_CONTROL_LLD+1)		/* Parameter: 0..n (as defined by the board file) */
#define GDISP_CONTROL_SETBORDER			(GDISP_CONTROL_LLD+2)		/* Parameter: 0=Border Hi-Z, 1=Border Black, 2=Border White */

#endif	/* GFX_USE_GDISP */

#endif	/* _GDISP_LLD_CONFIG_H */
