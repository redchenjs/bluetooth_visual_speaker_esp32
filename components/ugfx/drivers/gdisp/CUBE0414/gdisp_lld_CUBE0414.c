/*
 * gdisp_lld_CUBE0414.c
 *
 *  Created on: 2018-05-10 16:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "gfx.h"

#if GFX_USE_GDISP

#if defined(GDISP_SCREEN_WIDTH) || defined(GDISP_SCREEN_HEIGHT)
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

#ifndef GDISP_SCREEN_WIDTH
    #define GDISP_SCREEN_WIDTH      CUBE0414_X*CUBE0414_Y
#endif
#ifndef GDISP_SCREEN_HEIGHT
    #define GDISP_SCREEN_HEIGHT     CUBE0414_Z
#endif
#ifndef GDISP_INITIAL_CONTRAST
    #define GDISP_INITIAL_CONTRAST  100
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
    #define GDISP_INITIAL_BACKLIGHT 100
#endif

#define GDISP_FLG_NEEDFLUSH         (GDISP_FLG_DRIVER << 0)

#include "CUBE0414.h"

static const uint8_t ram_addr_table[64] = {
#ifdef CONFIG_CUBE0414_LINE_S_CURVE
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0f,
    0x10, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x1f,
    0x20, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2f,
    0x30, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x3f,
    0x00, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
#else
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x00,
#endif
};

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
    g->priv = gfxAlloc(GDISP_SCREEN_WIDTH*GDISP_SCREEN_HEIGHT*3);
    if (g->priv == NULL) {
        gfxHalt("GDISP CUBE0414: Failed to allocate private memory");
    }

    memset(g->priv, 0x00, GDISP_SCREEN_WIDTH*GDISP_SCREEN_HEIGHT*3);

    // Initialise the board interface
    init_board(g);

    // Hardware reset
    setpin_reset(g, 0);
    gfxSleepMilliseconds(120);
    setpin_reset(g, 1);
    gfxSleepMilliseconds(120);

    write_cmd(g, CUBE0414_CONF_WR);     //  1: Set write reg conf, 6 args, no delay:
        write_data(g, CONFIG_CUBE0414_LED_T1H / 10);    // T1H Time (ns)
        write_data(g, CONFIG_CUBE0414_LED_T1L / 10);    // T1L Time (ns)
        write_data(g, CONFIG_CUBE0414_LED_T0H / 10);    // T0H Time (ns)
        write_data(g, CONFIG_CUBE0414_LED_T0L / 10);    // T0L Time (ns)
        write_data(g, CONFIG_CUBE0414_LED_RST * 100 >> 8);    // RST Time H (us)
        write_data(g, CONFIG_CUBE0414_LED_RST * 100 & 0xff);  // RST Time L (us)
    write_cmd(g, CUBE0414_ADDR_WR);     //  2: Set write ram addr, 64 args, no delay:
        write_buff(g, (uint8_t *)ram_addr_table, sizeof(ram_addr_table));
    write_cmd(g, CUBE0414_DATA_WR);     //  3: Set write ram data, N args, no delay:
        write_buff(g, (uint8_t *)g->priv, GDISP_SCREEN_WIDTH*GDISP_SCREEN_HEIGHT*3);

    /* Initialise the GDISP structure */
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
        refresh_gram(g, (uint8_t *)g->priv);
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
#ifdef CONFIG_CUBE0414_COLOR_GRB
        *((uint8_t *)g->priv + pos * 3 + 0) = c >> 8;
        *((uint8_t *)g->priv + pos * 3 + 1) = c >> 16;
        *((uint8_t *)g->priv + pos * 3 + 2) = c;
#else
        *((uint8_t *)g->priv + pos * 3 + 0) = c >> 16;
        *((uint8_t *)g->priv + pos * 3 + 1) = c >> 8;
        *((uint8_t *)g->priv + pos * 3 + 2) = c;
#endif
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

#if GDISP_HARDWARE_STREAM_READ
    static uint8_t read_x  = 0;
    static uint8_t read_cx = 0;
    static uint8_t read_y  = 0;
    static uint8_t read_cy = 0;
    LLDSPEC void gdisp_lld_read_start(GDisplay *g) {
        read_x  = g->p.x;
        read_cx = g->p.cx;
        read_y  = g->p.y;
        read_cy = g->p.cy;
    }
    LLDSPEC color_t gdisp_lld_read_color(GDisplay *g) {
        uint16_t pos = read_y * g->g.Width + read_x;
#ifdef CONFIG_CUBE0414_COLOR_GRB
        LLDCOLOR_TYPE c = (*((uint8_t *)g->priv + pos * 3 + 0) << 8)
                        | (*((uint8_t *)g->priv + pos * 3 + 1) << 16)
                        | (*((uint8_t *)g->priv + pos * 3 + 2));
#else
        LLDCOLOR_TYPE c = (*((uint8_t *)g->priv + pos * 3 + 0) << 16)
                        | (*((uint8_t *)g->priv + pos * 3 + 1) << 8)
                        | (*((uint8_t *)g->priv + pos * 3 + 2));
#endif
        read_x++;
        if (--read_cx == 0) {
            read_x  = g->p.x;
            read_cx = g->p.cx;
            read_y++;
            if (--read_cy == 0) {
                read_y  = g->p.y;
                read_cy = g->p.cy;
            }
        }
        return c;
    }
    LLDSPEC void gdisp_lld_read_stop(GDisplay *g) {}
#endif

#endif /* GFX_USE_GDISP */
