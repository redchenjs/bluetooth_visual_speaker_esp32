/*
 * gdisp_lld_WS2812.c
 *
 *  Created on: 2018-05-10 16:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "gfx.h"

#if GFX_USE_GDISP && defined(CONFIG_VFX_OUTPUT_WS2812)

#if defined(GDISP_SCREEN_WIDTH) || defined(GDISP_SCREEN_HEIGHT)
    #if GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_DIRECT
        #warning "GDISP: This low level driver does not support setting a screen size. It is being ignored."
    #elif GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_MACRO
        COMPILER_WARNING("GDISP: This low level driver does not support setting a screen size. It is being ignored.")
    #endif
    #undef GDISP_SCREEN_WIDTH
    #undef GDISP_SCREEN_HEIGHT
#endif

#define GDISP_DRIVER_VMT            GDISPVMT_WS2812
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

#include "gdisp_lld_board.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_WIDTH
    #define GDISP_SCREEN_WIDTH      WS2812_X * WS2812_Y
#endif
#ifndef GDISP_SCREEN_HEIGHT
    #define GDISP_SCREEN_HEIGHT     WS2812_Z
#endif
#ifndef GDISP_INITIAL_CONTRAST
    #define GDISP_INITIAL_CONTRAST  100
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
    #define GDISP_INITIAL_BACKLIGHT 100
#endif

#define GDISP_FLG_NEEDFLUSH         (GDISP_FLG_DRIVER << 0)

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
    // initialise the board interface
    init_board(g);

    // clear
    clear(g);

    /* initialise the GDISP structure */
    g->g.Width  = GDISP_SCREEN_WIDTH;
    g->g.Height = GDISP_SCREEN_HEIGHT;
    g->g.Orientation = GDISP_ROTATE_0;
    g->g.Powermode = powerOn;
    g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
    g->g.Contrast  = GDISP_INITIAL_CONTRAST;

    return TRUE;
}

#if GDISP_HARDWARE_FLUSH
    LLDSPEC void gdisp_lld_flush(GDisplay *g) {
        if (!(g->flags & GDISP_FLG_NEEDFLUSH)) {
            return;
        }
        refresh(g);
        g->flags &= ~GDISP_FLG_NEEDFLUSH;
    }
#endif

#if GDISP_HARDWARE_STREAM_WRITE
    static uint8_t write_x  = 0;
    static uint8_t write_cx = 0;
    static uint8_t write_y  = 0;
    static uint8_t write_cy = 0;
    LLDSPEC void gdisp_lld_write_start(GDisplay *g) {
        write_x  = g->p.x;
        write_cx = g->p.cx;
        write_y  = g->p.y;
        write_cy = g->p.cy;
    }
    LLDSPEC void gdisp_lld_write_color(GDisplay *g) {
        uint16_t pos = write_y * g->g.Width + write_x;
        LLDCOLOR_TYPE c = gdispColor2Native(g->p.color);
#ifdef CONFIG_LED_COLOR_ORDER_GRB
        uint8_t red   = c >> 16;
        uint8_t green = c >> 8;
        uint8_t blue  = c;
#else
        uint8_t red   = c >> 8;
        uint8_t green = c >> 16;
        uint8_t blue  = c;
#endif
        set_pixel(g, pos, red, green, blue);
        write_x++;
        if (--write_cx == 0) {
            write_x  = g->p.x;
            write_cx = g->p.cx;
            write_y++;
            if (--write_cy == 0) {
                write_y  = g->p.y;
                write_cy = g->p.cy;
            }
        }
    }
    LLDSPEC void gdisp_lld_write_stop(GDisplay *g) {
        g->flags |= GDISP_FLG_NEEDFLUSH;
    }
#endif

#endif /* GFX_USE_GDISP */
