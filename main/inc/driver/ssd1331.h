/*
 * ssd1331.c
 *
 *  Created on: 2018-02-10 15:55
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef SSD1331_H_
#define SSD1331_H_

enum ssd1331_panel_value {
    SSD1331_WIDTH  = 96,
    SSD1331_HEIGHT = 64
};

enum ssd1331_data_type {
    SSD1331_CMD  = 0,
    SSD1331_DATA = 1
};

enum ssd1331_fundamental_command_table {
    SET_COLUMN_ADDRESS  = 0x15,
    SET_ROW_ADDRESS     = 0x75,

    SET_CONTRAST_A          = 0x81,
    SET_CONTRAST_B          = 0x82,
    SET_CONTRAST_C          = 0x83,
    SET_MASTER_CURRENT      = 0x87,
    SET_PRECHARGE_SPEED_A   = 0x8A,
    SET_PRECHARGE_SPEED_B   = 0x8B,
    SET_PRECHARGE_SPEED_C   = 0x8C,

    SET_REMAP_COLOR_DEPTH   = 0xA0,
    SET_DISPLAY_START_LINE  = 0xA1,
    SET_DISPLAY_OFFSET      = 0xA2,

    SET_NORMAL_DISPLAY      = 0xA4,
    SET_ENTIRE_DISPLAY_ON   = 0xA5,
    SET_ENTIRE_DISPLAY_OFF  = 0xA6,
    SET_INVERSE_DISPLAY     = 0xA7,

    SET_MULTIPLEX_RATIO     = 0xA8,
    SET_DIM_MODE            = 0xAB,
    SET_MASTER_CONFIG       = 0xAD,

    SET_DISPLAY_ON_DIM      = 0xAC,
    SET_DISPLAY_OFF         = 0xAE,
    SET_DISPLAY_ON_NORMAL   = 0xAF,

    SET_POWER_SAVE_MODE     = 0xB0,
    SET_PHASE_PERIOD_ADJ    = 0xB1,
    SET_DISPLAY_CLOCK_DIV   = 0xB3,
    SET_GRAY_SCALE_TABLE    = 0xB8,
    SET_BUILTIN_LINEAR_LUT  = 0xB9,
    SET_PRECHARGE_LEVEL     = 0xBB,
    SET_VCOMH_VOLTAGE       = 0xBE,

    SET_COMMAND_LOCK    = 0xFD
};

enum ssd1331_graphic_acceleration_command_table {
    DRAW_LINE       = 0x21,
    DRAW_RECTANGLE  = 0x22,
    COPY_WINDOW     = 0x23,
    DIM_WINDOW      = 0x24,
    CLEAR_WINDOW    = 0x25,
    SET_FILL_MODE   = 0x26,

    CONTINUOUS_SCROLLING_SETUP  = 0x27,
    DEACTIVATE_SCROLLING        = 0x2E,
    ACTIVATE_SCROLLING          = 0x2F
};

#include <stdint.h>

extern void ssd1331_refresh_gram(uint8_t *gram);

extern void ssd1331_write_byte(unsigned char chData, unsigned char chCmd);

extern void ssd1331_draw_point(unsigned char chXpos, unsigned char chYpos, unsigned int hwColor);
extern void ssd1331_draw_line(unsigned char chXpos0, unsigned char chYpos0, unsigned char chXpos1, unsigned char chYpos1, unsigned int hwColor);
extern void ssd1331_draw_v_line(unsigned char chXpos, unsigned char chYpos, unsigned char chHeight, unsigned int hwColor);
extern void ssd1331_draw_h_line(unsigned char chXpos, unsigned char chYpos, unsigned char chWidth, unsigned int hwColor);
extern void ssd1331_draw_rect(unsigned char chXpos, unsigned char chYpos, unsigned char chWidth, unsigned char chHeight, unsigned int hwColor);
extern void ssd1331_draw_circle(unsigned char chXpos, unsigned char chYpos, unsigned char chRadius, unsigned int hwColor);

extern void ssd1331_draw_mono_bitmap(unsigned char chXpos, unsigned char chYpos, const unsigned char *pchBmp, unsigned char chWidth, unsigned char chHeight, unsigned int hwForeColor, unsigned int hwBackColor);
extern void ssd1331_draw_64k_bitmap(unsigned char chXpos, unsigned char chYpos, const unsigned char *pchBmp, unsigned char chWidth, unsigned char chHeight);

extern void ssd1331_fill_rect(unsigned char chXpos, unsigned char chYpos, unsigned char chWidth, unsigned char chHeight, unsigned int hwColor);
extern void ssd1331_fill_gram(unsigned int hwColor);

extern void ssd1331_clear_rect(unsigned char chXpos, unsigned char chYpos, unsigned char chWidth, unsigned char chHeight);
extern void ssd1331_clear_gram(void);

extern void ssd1331_display_char(unsigned char chXpos, unsigned char chYpos, unsigned char chChr, unsigned char chFontIndex, unsigned int hwForeColor, unsigned int hwBackColor);
extern void ssd1331_display_num(unsigned char chXpos, unsigned char chYpos, unsigned long chNum, unsigned char chLen, unsigned char chFontIndex, unsigned int hwForeColor, unsigned int hwBackColor);
extern void ssd1331_display_string(unsigned char chXpos, unsigned char chYpos, const char *pchString, unsigned char chFontIndex, unsigned int hwForeColor, unsigned int hwBackColor);

extern void ssd1331_continuous_scrolling(unsigned char chYpos, unsigned char chHeight, unsigned char chDirection, unsigned char chInterval);
extern void ssd1331_deactivate_scrolling(void);

extern void ssd1331_show_checkerboard(void);
extern void ssd1331_show_rainbow(void);

extern void ssd1331_set_gray_scale_table(void);

extern void ssd1331_init(void);

#endif
