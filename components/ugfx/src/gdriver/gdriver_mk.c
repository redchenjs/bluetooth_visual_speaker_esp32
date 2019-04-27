/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gdriver.c"

#undef GDISP_DRIVER_LIST

#ifdef CONFIG_VFX_OUTPUT_ST7735
    #define GDISP_DRIVER_LIST GDISPVMT_ST7735
    #include "drivers/gdisp/ST7735/gdisp_lld_ST7735.c"
#else
    #define GDISP_DRIVER_LIST GDISPVMT_CUBE0414
    #include "drivers/gdisp/CUBE0414/gdisp_lld_CUBE0414.c"
#endif
