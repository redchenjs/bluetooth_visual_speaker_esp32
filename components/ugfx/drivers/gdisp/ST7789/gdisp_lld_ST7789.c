/*
 * gdisp_lld_ST7789.c
 *
 *  Created on: 2019-04-29 22:04
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

#define GDISP_DRIVER_VMT            GDISPVMT_ST7789
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

#include "gdisp_lld_board.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
    #define GDISP_SCREEN_HEIGHT     ST7789_SCREEN_HEIGHT
#endif
#ifndef GDISP_SCREEN_WIDTH
    #define GDISP_SCREEN_WIDTH      ST7789_SCREEN_WIDTH
#endif
#ifndef GDISP_INITIAL_CONTRAST
    #define GDISP_INITIAL_CONTRAST  100
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
    #define GDISP_INITIAL_BACKLIGHT 255
#endif

#define GDISP_FLG_NEEDFLUSH         (GDISP_FLG_DRIVER<<0)

#include "ST7789.h"

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
    g->priv = gfxAlloc(GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH * 2);
    if (g->priv == NULL) {
        gfxHalt("GDISP ST7789: Failed to allocate private memory");
    }

    for (int i=0; i<GDISP_SCREEN_HEIGHT*GDISP_SCREEN_WIDTH*2; i++) {
        *((uint8_t *)g->priv + i) = 0x00;
    }

    // Initialise the board interface
    init_board(g);

    // Hardware reset
    setpin_reset(g, 0);
    gfxSleepMilliseconds(120);
    setpin_reset(g, 1);
    gfxSleepMilliseconds(120);

    write_cmd(g, ST7789_SWRESET);   //  1: Software reset, no args, w/delay
    gfxSleepMilliseconds(120);
    write_cmd(g, ST7789_SLPOUT);    //  2: Out of sleep mode, no args, w/delay
    gfxSleepMilliseconds(120);
    write_cmd(g, ST7789_PORCTRL);   //  3: Porch setting, 5 args, no delay:
        write_data(g, 0x0C);
        write_data(g, 0x0C);
        write_data(g, 0x00);
        write_data(g, 0x33);
        write_data(g, 0x33);
    write_cmd(g, ST7789_GCTRL);     //  4: Gate control, 1 arg, no delay:
        write_data(g, 0x35);
    write_cmd(g, ST7789_VCOMS);     //  5: VCOM setting, 1 arg, no delay:
        write_data(g, 0x19);
    write_cmd(g, ST7789_LCMCTRL);   //  6: LCM control, 1 arg, no delay:
        write_data(g, 0x2C);
    write_cmd(g, ST7789_VDVVRHEN);  //  7: VDV and VRH command enable, 1 arg, no delay:
        write_data(g, 0x01);
    write_cmd(g, ST7789_VRHS);      //  8: VRH setting, 1 arg, no delay:
        write_data(g, 0x12);
    write_cmd(g, ST7789_VDVSET);    //  9: VDV setting, 1 arg, no delay:
        write_data(g, 0x20);
    write_cmd(g, ST7789_FRCTRL2);   // 10: Frame rate control - normal mode, 1 arg:
        write_data(g, 0x01);
    write_cmd(g, ST7789_PWCTRL1);   // 11: Power control 1, 2 args, no delay:
        write_data(g, 0xA4);
        write_data(g, 0xA1);
    write_cmd(g, ST7789_INVON);     // 12: Invert display, no args, no delay
    write_cmd(g, ST7789_MADCTL);    // 13: Memory access control (directions), 1 arg:
        write_data(g, 0x70);
    write_cmd(g, ST7789_COLMOD);    // 14: Set color mode, 1 arg, no delay:
        write_data(g, 0x05);
    write_cmd(g, ST7789_PVGAMCTRL); // 15: Positive voltage gamma control, 14 args, no delay:
        write_data(g, 0xD0);
        write_data(g, 0x04);
        write_data(g, 0x0D);
        write_data(g, 0x11);
        write_data(g, 0x13);
        write_data(g, 0x2B);
        write_data(g, 0x3F);
        write_data(g, 0x54);
        write_data(g, 0x4C);
        write_data(g, 0x18);
        write_data(g, 0x0D);
        write_data(g, 0x0B);
        write_data(g, 0x1F);
        write_data(g, 0x23);
    write_cmd(g, ST7789_NVGAMCTRL); // 16: Negative voltage gamma control, 14 args, no delay:
        write_data(g, 0xD0);
        write_data(g, 0x04);
        write_data(g, 0x0C);
        write_data(g, 0x11);
        write_data(g, 0x13);
        write_data(g, 0x2C);
        write_data(g, 0x3F);
        write_data(g, 0x44);
        write_data(g, 0x51);
        write_data(g, 0x2F);
        write_data(g, 0x1F);
        write_data(g, 0x1F);
        write_data(g, 0x20);
        write_data(g, 0x23);
    write_cmd(g, ST7789_NORON);     // 17: Normal display on, no args, no delay
    write_cmd(g, ST7789_CASET);     // 18: Set column address, 4 args, no delay:
        write_data(g, 0x00);
        write_data(g, 0x28);
        write_data(g, 0x01);
        write_data(g, 0x17);
    write_cmd(g, ST7789_RASET);     // 19: Set row address, 4 args, no delay:
        write_data(g, 0x00);
        write_data(g, 0x35);
        write_data(g, 0x00);
        write_data(g, 0xBB);
    write_cmd(g, ST7789_RAMWR);     // 20: Set write ram, N args, no delay:
        write_buff(g, (uint8_t *)g->priv, GDISP_SCREEN_HEIGHT*GDISP_SCREEN_WIDTH*2);
    write_cmd(g, ST7789_DISPON);    // 21: Main screen turn on, no args, no delay

    /* Initialise the GDISP structure */
    g->g.Width  = GDISP_SCREEN_HEIGHT;
    g->g.Height = GDISP_SCREEN_WIDTH;
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
    static int16_t stream_write_x  = 0;
    static int16_t stream_write_cx = 0;
    static int16_t stream_write_y  = 0;
    static int16_t stream_write_cy = 0;
    LLDSPEC void gdisp_lld_write_start(GDisplay *g) {
        stream_write_x  = g->p.x;
        stream_write_cx = g->p.cx;
        stream_write_y  = g->p.y;
        stream_write_cy = g->p.cy;
    }
    LLDSPEC void gdisp_lld_write_color(GDisplay *g) {
        LLDCOLOR_TYPE c = gdispColor2Native(g->p.color);
        *((uint8_t *)g->priv + (stream_write_x + stream_write_y * g->g.Width) * 2 + 0) = c >> 8;
        *((uint8_t *)g->priv + (stream_write_x + stream_write_y * g->g.Width) * 2 + 1) = c;
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
    static int16_t stream_read_x  = 0;
    static int16_t stream_read_cx = 0;
    static int16_t stream_read_y  = 0;
    static int16_t stream_read_cy = 0;
    LLDSPEC void gdisp_lld_read_start(GDisplay *g) {
        stream_read_x  = g->p.x;
        stream_read_cx = g->p.cx;
        stream_read_y  = g->p.y;
        stream_read_cy = g->p.cy;
    }
    LLDSPEC color_t gdisp_lld_read_color(GDisplay *g) {
        LLDCOLOR_TYPE c = (*((uint8_t *)g->priv + (stream_read_x + stream_read_y * g->g.Width) * 2 + 0) << 8)
                        | (*((uint8_t *)g->priv + (stream_read_x + stream_read_y * g->g.Width) * 2 + 1));
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
        switch((powermode_t)g->p.ptr) {
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
        switch((orientation_t)g->p.ptr) {
            case GDISP_ROTATE_0:
                g->g.Height = GDISP_SCREEN_HEIGHT;
                g->g.Width  = GDISP_SCREEN_WIDTH;
                break;
            case GDISP_ROTATE_90:
                g->g.Height = GDISP_SCREEN_WIDTH;
                g->g.Width  = GDISP_SCREEN_HEIGHT;
                break;
            case GDISP_ROTATE_180:
                g->g.Height = GDISP_SCREEN_HEIGHT;
                g->g.Width  = GDISP_SCREEN_WIDTH;
                break;
            case GDISP_ROTATE_270:
                g->g.Height = GDISP_SCREEN_WIDTH;
                g->g.Width  = GDISP_SCREEN_HEIGHT;
                break;
            default:
                return;
        }
        g->g.Orientation = (orientation_t)g->p.ptr;
        return;
    case GDISP_CONTROL_BACKLIGHT:
        if ((unsigned)g->p.ptr > 255)
            g->p.ptr = (void *)255;
        g->g.Backlight = (unsigned)g->p.ptr;
        set_backlight(g, g->g.Backlight);
        return;
    default:
        return;
    }
}
#endif

#endif /* GFX_USE_GDISP */
