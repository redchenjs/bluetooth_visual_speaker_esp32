/*
 * Created by Oleg Gerasimov <ogerasimov@gmail.com>
 * 06.08.2016
 */

#ifndef _GDISP_LLD_CONFIG_H
#define _GDISP_LLD_CONFIG_H

#if GFX_USE_GDISP

/*===========================================================================*/
/* Driver hardware support.                                                  */
/*===========================================================================*/

#define GDISP_HARDWARE_DRAWPIXEL		TRUE
#define GDISP_HARDWARE_FILLS			TRUE
#define GDISP_HARDWARE_BITFILLS			FALSE
#define GDISP_HARDWARE_SCROLL			FALSE
#define GDISP_HARDWARE_PIXELREAD		TRUE
#define GDISP_HARDWARE_CONTROL			FALSE

#define GDISP_LLD_PIXELFORMAT			GDISP_PIXELFORMAT_RGB888

#endif	/* GFX_USE_GDISP */

#endif	/* _GDISP_LLD_CONFIG_H */
