/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gdriver.c"

#if defined(CONFIG_OLED_PANEL_SSD1331)

#undef GDISP_DRIVER_LIST
#undef GDISP_PIXELFORMAT
#define GDISP_DRIVER_LIST GDISPVMT_SSD1331
#define GDISP_PIXELFORMAT GDISP_PIXELFORMAT_RGB565
#include "../../drivers/gdisp/SSD1331/gdisp_lld_SSD1331.c"

#elif defined(CONFIG_OLED_PANEL_SSD1351)

#undef GDISP_DRIVER_LIST
#undef GDISP_PIXELFORMAT
#define GDISP_DRIVER_LIST GDISPVMT_SSD1351
#define GDISP_PIXELFORMAT GDISP_PIXELFORMAT_RGB565
#include "../../drivers/gdisp/SSD1351/gdisp_lld_SSD1351.c"

#else

#include "../../drivers/gdisp/framebuffer/gdisp_lld_framebuffer.c"

#endif
