/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
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

#define GDISP_DRIVER_VMT			GDISPVMT_ST7735
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

#include "board_ST7735.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
    #define GDISP_SCREEN_HEIGHT		160
#endif
#ifndef GDISP_SCREEN_WIDTH
    #define GDISP_SCREEN_WIDTH		80
#endif
#ifndef GDISP_INITIAL_CONTRAST
    #define GDISP_INITIAL_CONTRAST	100
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
    #define GDISP_INITIAL_BACKLIGHT	100
#endif

#define GDISP_FLG_NEEDFLUSH         (GDISP_FLG_DRIVER<<0)

#include "ST7735.h"

// Some common routines and macros
#define write_reg(g, reg, data)		{ write_cmd(g, reg); write_data(g, data); }

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
    g->priv = gfxAlloc(GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH * 2);
    if (g->priv == NULL) {
        gfxHalt("GDISP ST7735: Failed to allocate private memory");
    }

    for(int i=0; i < GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH * 2; i++) {
        *((uint8_t *)g->priv + i) = 0x00;
    }

    // Initialise the board interface
    init_board(g);

    // Hardware reset
    setpin_reset(g, 0);
    gfxSleepMilliseconds(120);
    setpin_reset(g, 1);
    gfxSleepMilliseconds(120);

    write_cmd(g, ST7735_SWRESET);   //  1: Software reset, 0 args, w/delay
    gfxSleepMilliseconds(120);
    write_cmd(g, ST7735_SLPOUT);    //  2: Out of sleep mode, 0 args, w/delay
    gfxSleepMilliseconds(120);
    write_cmd(g, ST7735_FRMCTR1);   //  3: Frame rate ctrl - normal mode, 3 args:
        write_data(g, 0x05);
        write_data(g, 0x3C);
        write_data(g, 0x3C);
    write_cmd(g, ST7735_FRMCTR2);   //  4: Frame rate control - idle mode, 3 args:
        write_data(g, 0x05);
        write_data(g, 0x3C);
        write_data(g, 0x3C);
    write_cmd(g, ST7735_FRMCTR3);   //  5: Frame rate ctrl - partial mode, 6 args:
        write_data(g, 0x05);
        write_data(g, 0x3C);
        write_data(g, 0x3C);
        write_data(g, 0x05);
        write_data(g, 0x3C);
        write_data(g, 0x3C);
    write_cmd(g, ST7735_INVCTR);    //  6: Display inversion ctrl, 1 arg, no delay:
        write_data(g, 0x03);
    write_cmd(g, ST7735_PWCTR1);    //  7: Power control, 3 args, no delay:
        write_data(g, 0x0E);
        write_data(g, 0x0E);
        write_data(g, 0x04);
    write_cmd(g, ST7735_PWCTR2);    //  8: Power control, 1 arg, no delay:
        write_data(g, 0xC5);
    write_cmd(g, ST7735_PWCTR3);    //  9: Power control, 2 args, no delay:
        write_data(g, 0x0D);
        write_data(g, 0x00);
    write_cmd(g, ST7735_PWCTR4);    // 10: Power control, 2 args, no delay:
        write_data(g, 0x8D);
        write_data(g, 0x2A);
    write_cmd(g, ST7735_PWCTR5);    // 11: Power control, 2 args, no delay:
        write_data(g, 0x8D);
        write_data(g, 0xEE);
    write_cmd(g, ST7735_VMCTR1);    // 12: Power control, 1 arg, no delay:
        write_data(g, 0x06);
    write_cmd(g, ST7735_INVON);     // 13: Invert display, no args, no delay
    write_cmd(g, ST7735_MADCTL);    // 14: Memory access control (directions), 1 arg:
        write_data(g, 0x68);
    write_cmd(g, ST7735_COLMOD);    // 15: set color mode, 1 arg, no delay:
        write_data(g, 0x05);
    write_cmd(g, ST7735_GMCTRP1);   // 16: Magical unicorn dust, 16 args, no delay:
        write_data(g, 0x0B);
        write_data(g, 0x17);
        write_data(g, 0x0A);
        write_data(g, 0x0D);
        write_data(g, 0x1A);
        write_data(g, 0x19);
        write_data(g, 0x16);
        write_data(g, 0x1D);
        write_data(g, 0x21);
        write_data(g, 0x26);
        write_data(g, 0x37);
        write_data(g, 0x3C);
        write_data(g, 0x00);
        write_data(g, 0x09);
        write_data(g, 0x05);
        write_data(g, 0x10);
    write_cmd(g, ST7735_GMCTRN1);   // 17: Sparkles and rainbows, 16 args, no delay:
        write_data(g, 0x0C);
        write_data(g, 0x19);
        write_data(g, 0x09);
        write_data(g, 0x0D);
        write_data(g, 0x1B);
        write_data(g, 0x19);
        write_data(g, 0x15);
        write_data(g, 0x1D);
        write_data(g, 0x21);
        write_data(g, 0x26);
        write_data(g, 0x39);
        write_data(g, 0x3E);
        write_data(g, 0x00);
        write_data(g, 0x09);
        write_data(g, 0x05);
        write_data(g, 0x10);
    gfxSleepMilliseconds(120);
    write_cmd(g, ST7735_NORON);     // 18: Normal display on, no args, w/delay
    write_cmd(g, ST7735_DISPON);    // 19: Main screen turn on, no args w/delay

    /* Initialise the GDISP structure */
    g->g.Height = GDISP_SCREEN_WIDTH;
    g->g.Width = GDISP_SCREEN_HEIGHT;
    g->g.Orientation = GDISP_ROTATE_0;
    g->g.Powermode = powerOn;
    g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
    g->g.Contrast = GDISP_INITIAL_CONTRAST;
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
    LLDSPEC	void gdisp_lld_write_start(GDisplay *g) {
    }
    LLDSPEC	void gdisp_lld_write_color(GDisplay *g) {
        LLDCOLOR_TYPE c;
        c = gdispColor2Native(g->p.color);
        *((uint8_t *)g->priv + (g->p.y * g->g.Width + g->p.x) * 2 + 0) = c >> 8;
        *((uint8_t *)g->priv + (g->p.y * g->g.Width + g->p.x) * 2 + 1) = c;
    }
    LLDSPEC	void gdisp_lld_write_stop(GDisplay *g) {
        g->flags |= GDISP_FLG_NEEDFLUSH;
    }
#endif

#if GDISP_HARDWARE_STREAM_READ
    LLDSPEC	void gdisp_lld_read_start(GDisplay *g) {
    }
    LLDSPEC	color_t gdisp_lld_read_color(GDisplay *g) {
        return (*((uint8_t *)g->priv + (g->p.y * g->g.Width + g->p.x) * 2 + 0) << 8) |
               (*((uint8_t *)g->priv + (g->p.y * g->g.Width + g->p.x) * 2 + 1));
    }
    LLDSPEC	void gdisp_lld_read_stop(GDisplay *g) {
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
