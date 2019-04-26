/*
 * gdisp_lld_CUBE0414.c
 *
 *  Created on: 2018-05-10 16:57
 *      Author: Jack Chen <redchenjs@live.com>
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

#define GDISP_DRIVER_VMT            GDISPVMT_CUBE0414
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

#include "gdisp_lld_board.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
    #define GDISP_SCREEN_HEIGHT     CUBE0414_Z
#endif
#ifndef GDISP_SCREEN_WIDTH
    #define GDISP_SCREEN_WIDTH      CUBE0414_X*CUBE0414_Y
#endif
#ifndef GDISP_INITIAL_CONTRAST
    #define GDISP_INITIAL_CONTRAST  100
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
    #define GDISP_INITIAL_BACKLIGHT 100
#endif

#define GDISP_FLG_NEEDFLUSH         (GDISP_FLG_DRIVER<<0)

// Some common routines and macros
#define write_reg(g, reg, data)     { write_cmd(g, reg); write_data(g, data); }

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
    g->priv = gfxAlloc(GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH * 3);
    if (g->priv == NULL) {
        gfxHalt("GDISP CUBE0414: Failed to allocate private memory");
    }

    for(int i=0; i<GDISP_SCREEN_HEIGHT*GDISP_SCREEN_WIDTH*3; i++) {
        *((uint8_t *)g->priv + i) = 0x00;
    }

    // Initialise the board interface
    init_board(g);

    /* Initialise the GDISP structure */
    g->g.Height = GDISP_SCREEN_HEIGHT;
    g->g.Width  = GDISP_SCREEN_WIDTH;
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
        refresh_gram(g, (uint8_t *)g->priv);
        g->flags &= ~GDISP_FLG_NEEDFLUSH;
    }
#endif

#if GDISP_HARDWARE_STREAM_WRITE
    static int8_t stream_write_x  = 0;
    static int8_t stream_write_cx = 0;
    static int8_t stream_write_y  = 0;
    static int8_t stream_write_cy = 0;
    LLDSPEC void gdisp_lld_write_start(GDisplay *g) {
        stream_write_x  = g->p.x;
        stream_write_cx = g->p.cx;
        stream_write_y  = g->p.y;
        stream_write_cy = g->p.cy;
    }
    LLDSPEC void gdisp_lld_write_color(GDisplay *g) {
        LLDCOLOR_TYPE c = gdispColor2Native(g->p.color);
        *((uint8_t *)g->priv + (stream_write_x + stream_write_y * g->g.Width) * 3 + 0) = c >> 16;
        *((uint8_t *)g->priv + (stream_write_x + stream_write_y * g->g.Width) * 3 + 1) = c >> 8;
        *((uint8_t *)g->priv + (stream_write_x + stream_write_y * g->g.Width) * 3 + 2) = c;
        stream_write_x++;
        if (--stream_write_cx <= 0) {
            stream_write_x  = g->p.x;
            stream_write_cx = g->p.cx;
            stream_write_y++;
            if (--stream_write_cy <= 0) {
                stream_write_y  = g->p.y;
                stream_write_cy = g->p.cy;
            }
        }
    }
    LLDSPEC void gdisp_lld_write_stop(GDisplay *g) {
        stream_write_x  = 0;
        stream_write_cx = 0;
        stream_write_y  = 0;
        stream_write_cy = 0;
        g->flags |= GDISP_FLG_NEEDFLUSH;
    }
#endif

#if GDISP_HARDWARE_STREAM_READ
    static int8_t stream_read_x  = 0;
    static int8_t stream_read_cx = 0;
    static int8_t stream_read_y  = 0;
    static int8_t stream_read_cy = 0;
    LLDSPEC void gdisp_lld_read_start(GDisplay *g) {
        stream_read_x  = g->p.x;
        stream_read_cx = g->p.cx;
        stream_read_y  = g->p.y;
        stream_read_cy = g->p.cy;
    }
    LLDSPEC color_t gdisp_lld_read_color(GDisplay *g) {
        LLDCOLOR_TYPE c = (*((uint8_t *)g->priv + (stream_read_x + stream_read_y * g->g.Width) * 3 + 0) << 16)
                        | (*((uint8_t *)g->priv + (stream_read_x + stream_read_y * g->g.Width) * 3 + 1) << 8)
                        | (*((uint8_t *)g->priv + (stream_read_x + stream_read_y * g->g.Width) * 3 + 2));
        stream_read_x++;
        if (--stream_read_cx <= 0) {
            stream_read_x  = g->p.x;
            stream_read_cx = g->p.cx;
            stream_read_y++;
            if (--stream_read_cy <= 0) {
                stream_read_y  = g->p.y;
                stream_read_cy = g->p.cy;
            }
        }
        return c;
    }
    LLDSPEC void gdisp_lld_read_stop(GDisplay *g) {
        stream_read_x  = 0;
        stream_read_cx = 0;
        stream_read_y  = 0;
        stream_read_cy = 0;
    }
#endif

#if GDISP_NEED_CONTROL && GDISP_HARDWARE_CONTROL
LLDSPEC void gdisp_lld_control(GDisplay *g) {
    switch(g->p.x) {
    case GDISP_CONTROL_POWER:
        if (g->g.Powermode == (powermode_t)g->p.ptr)
            return;
        switch ((powermode_t)g->p.ptr) {
            case powerOff:
            case powerSleep:
            case powerDeepSleep:
            case powerOn:
            default:
                return;
        }
        g->g.Powermode = (powermode_t)g->p.ptr;
        return;

    case GDISP_CONTROL_ORIENTATION:
        if (g->g.Orientation == (orientation_t)g->p.ptr)
            return;
        switch ((orientation_t)g->p.ptr) {
            case GDISP_ROTATE_0:
            case GDISP_ROTATE_90:
            case GDISP_ROTATE_180:
            case GDISP_ROTATE_270:
            default:
                return;
        }
        g->g.Orientation = (orientation_t)g->p.ptr;
        return;

    case GDISP_CONTROL_BACKLIGHT:
        if ((unsigned)g->p.ptr > 100)
            g->p.ptr = (void *)100;
        g->g.Backlight = (unsigned)g->p.ptr;
        return;
    default:
        return;
    }
}
#endif

#endif /* GFX_USE_GDISP */
