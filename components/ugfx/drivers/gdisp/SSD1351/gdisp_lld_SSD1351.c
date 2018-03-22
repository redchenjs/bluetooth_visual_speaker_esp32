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

#define GDISP_DRIVER_VMT			GDISPVMT_SSD1351
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

#include "board_SSD1351.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
    #define GDISP_SCREEN_HEIGHT		128
#endif
#ifndef GDISP_SCREEN_WIDTH
    #define GDISP_SCREEN_WIDTH		128
#endif
#ifndef GDISP_INITIAL_CONTRAST
    #define GDISP_INITIAL_CONTRAST	100
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
    #define GDISP_INITIAL_BACKLIGHT	100
#endif

#define GDISP_FLG_NEEDFLUSH			(GDISP_FLG_DRIVER<<0)

#include "SSD1351.h"

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

// Some common routines and macros
#define write_reg(g, reg, data)		{ write_cmd(g, reg); write_data(g, data); }

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

static const uint8_t gray_scale_table[] = {
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
    0x12, 0x13, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F,
    0x21, 0x23, 0x25, 0x27, 0x2A, 0x2D, 0x30, 0x33,
    0x36, 0x39, 0x3C, 0x3F, 0x42, 0x45, 0x48, 0x4C,
    0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68, 0x6C,
    0x70, 0x74, 0x78, 0x7D, 0x82, 0x87, 0x8C, 0x91,
    0x96, 0x9B, 0xA0, 0xA5, 0xAA, 0xAF, 0xB4
};

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
    g->priv = gfxAlloc(GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH * 2);
    if (g->priv == NULL) {
        gfxHalt("GDISP SSD1351: Failed to allocate private memory");
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

    write_reg(g, SSD1351_SET_COMMAND_LOCK, 0x12);   // unlock OLED driver IC
    write_reg(g, SSD1351_SET_COMMAND_LOCK, 0xB1);   // make commands A1, B1, B3, BB, BE, C1 accesible in unlocked state
    write_cmd(g, SSD1351_SET_SLEEP_ON);             // sleep mode ON (display off)
    write_reg(g, SSD1351_CLOCKDIV_OSCFREQ, 0xF1);   // Front clock divider / osc freq - Osc = 0xF; div = 2
    write_reg(g, SSD1351_SET_MUX_RATIO, 127);       // set MUX ratio
    write_reg(g, SSD1351_SET_REMAP, 0b01110100);    // Set re-map / color depth
    // [0] : address increment (0: horizontal, 1: vertical, reset 0)
    // [1] : column remap (0: 0..127, 1: 127..0, reset 0)
    // [2] : color remap (0: A->B->C, 1: C->B->A, reset 0)
    // [3] : reserved
    // [4] : column scan direction (0: top->down, 1: bottom->up, reset 0)
    // [5] : odd/even split COM (0: disable, 1: enable, reset 1)
    // [6..7] : color depth (00,01: 65k, 10: 262k, 11: 262k format 2)

    write_reg(g, SSD1351_SET_DISPLAY_START, 0x00);  // set display start line - 0
    write_reg(g, SSD1351_SET_DISPLAY_OFFSET, 0x00); // set display offset - 0
    write_reg(g, SSD1351_SET_GPIO, 0x00);           // set GPIO - both HiZ, input disabled
    write_reg(g, SSD1351_SET_FUNCTION_SELECT, 0x01);    // enable internal VDD regulator
    write_reg(g, SSD1351_SET_RESET_PRECHARGE, 0x32);    // set reset / pre-charge period - phase 2: 3 DCLKs, phase 1: 5 DCLKs
    write_reg(g, SSD1351_SET_VCOMH, 0x05);          // set VComH voltage - 0.82*Vcc
    write_reg(g, SSD1351_SET_PRECHARGE, 0x17);      // set pre-charge voltage - 0.6*Vcc
    write_cmd(g, SSD1351_SET_DISPLAY_MODE_RESET);   // set display mode: reset to normal display

    write_cmd(g, SSD1351_SET_CONTRAST);             // set contrast current for A,B,C
    write_data(g, 0xC8);
    write_data(g, 0x80);
    write_data(g, 0xC8);

    write_reg(g, SSD1351_MASTER_CONTRAST_CURRENT_CONTROL, 0x0F);    // master contrast current control - no change

    write_cmd(g, SSD1351_SET_VSL);                  // set segment low voltage
    write_data(g, 0xA0);    // external VSL
    write_data(g, 0xB5);    // hard value
    write_data(g, 0x55);    // hard value

    write_reg(g, SSD1351_SET_SECOND_PRECHARGE, 0x01);   // set second pre-charge period - 1 DCLKs

    write_cmd(g, SSD1351_DISPLAY_ENHANCEMENT);      // display enhancement
    write_data(g, 0xA4);    // enhance display performance
    write_data(g, 0x00);    // fixed
    write_data(g, 0x00);    // fixed

    write_cmd(g, SSD1351_WRITE_RAM);                // write to RAM

    write_cmd(g, SSD1351_LUT_GRAYSCALE);            // set pulse width for gray scale table
    for(int i=0;i<sizeof(gray_scale_table);i++)
        write_data(g, gray_scale_table[i]);

    write_cmd(g, SSD1351_SET_SLEEP_OFF);            // sleep mode OFF (display on)

    /* Initialise the GDISP structure */
    g->g.Width = GDISP_SCREEN_WIDTH;
    g->g.Height = GDISP_SCREEN_HEIGHT;
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
                write_cmd(g, SSD1351_SET_SLEEP_ON);
                break;
            case powerOn:
                write_cmd(g, SSD1351_SET_SLEEP_OFF);
                break;
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
            write_reg(g, SSD1351_MASTER_CONTRAST_CURRENT_CONTROL, ((unsigned)g->p.ptr*10)/63);
            g->g.Backlight = (unsigned)g->p.ptr;
            return;

        case GDISP_CONTROL_CONTRAST:
            return;
        default:
            return;
        }
    }
#endif

#endif /* GFX_USE_GDISP */
