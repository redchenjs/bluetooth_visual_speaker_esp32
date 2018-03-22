/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gdriver.c"

#if defined(CONFIG_SCREEN_PANEL_SSD1331)

#undef GDISP_DRIVER_LIST
#define GDISP_DRIVER_LIST GDISPVMT_SSD1331
#include "../../drivers/gdisp/SSD1331/gdisp_lld_SSD1331.c"

#elif defined(CONFIG_SCREEN_PANEL_SSD1351)

#undef GDISP_DRIVER_LIST
#define GDISP_DRIVER_LIST GDISPVMT_SSD1351
#include "../../drivers/gdisp/SSD1351/gdisp_lld_SSD1351.c"

#else

#undef GDISP_DRIVER_LIST
#define GDISP_DRIVER_LIST GDISPVMT_ST7735
#include "../../drivers/gdisp/ST7735/gdisp_lld_ST7735.c"

#endif
