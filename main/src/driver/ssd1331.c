/*
 * ssd1331.c
 *
 *  Created on: 2018-02-10 15:55
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "device/spi.h"

#include "driver/ssd1331.h"
#include "system/fonts.h"
/*
 * --------SSD1331--------
 */
#define abs(x) ((x)>0?(x):-(x))
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define SSD1331_GPIO_PIN_DC   23
#define SSD1331_GPIO_PIN_RST  14

#define SSD1331_PIN_SET()   do { \
                                gpio_set_level(SSD1331_GPIO_PIN_DC, 0);\
                                gpio_set_level(SSD1331_GPIO_PIN_RST, 0);\
                                gpio_set_direction(SSD1331_GPIO_PIN_DC,  GPIO_MODE_OUTPUT);\
                                gpio_set_direction(SSD1331_GPIO_PIN_RST, GPIO_MODE_OUTPUT);\
                                vTaskDelay(500 / portTICK_PERIOD_MS);\
                                gpio_set_level(SSD1331_GPIO_PIN_RST, 1);\
                            } while (0)

#define SSD1331_DC_SET()    do {\
                                spi1_t.length = 8;\
                                spi1_t.tx_buffer = &chData;\
                                spi1_t.user = (void*)1;\
                            } while (0)

#define SSD1331_DC_CLR()    do {\
                                spi1_t.length = 8;\
                                spi1_t.tx_buffer = &chData;\
                                spi1_t.user = (void*)0;\
                            } while (0)

#define SSD1331_WRITE_BYTE(__DATA)  do {\
                                        esp_err_t ret;\
                                        ret = spi_device_transmit(spi1, &spi1_t);\
                                        assert(ret == ESP_OK);\
                                    } while (0)
    
void ssd1331_set_dc_line(spi_transaction_t *);
void (*spi1_pre_transfer_callback)(spi_transaction_t *) = ssd1331_set_dc_line;

spi_transaction_t spi1_trans[3];

void ssd1331_refresh_gram(uint8_t *gram)
{
    esp_err_t ret;

    memset(spi1_trans, 0, sizeof(spi1_trans));

    spi1_trans[0].length = 3*8;
    spi1_trans[0].tx_data[0] = SET_COLUMN_ADDRESS;
    spi1_trans[0].tx_data[1] = 0x00;
    spi1_trans[0].tx_data[2] = SSD1331_WIDTH - 1;
    spi1_trans[0].user = (void*)0;
    spi1_trans[0].flags = SPI_TRANS_USE_TXDATA;

    spi1_trans[1].length = 3*8,
    spi1_trans[1].tx_data[0] = SET_ROW_ADDRESS;
    spi1_trans[1].tx_data[1] = 0x00;
    spi1_trans[1].tx_data[2] = SSD1331_HEIGHT - 1;
    spi1_trans[1].user = (void*)0;
    spi1_trans[1].flags = SPI_TRANS_USE_TXDATA;

    spi1_trans[2].length = 96*64*2*8;
    spi1_trans[2].tx_buffer = gram;
    spi1_trans[2].user = (void*)1;

    //Queue all transactions.
    for (int x=0; x<3; x++) {
        ret=spi_device_queue_trans(spi1, &spi1_trans[x], portMAX_DELAY);
        assert(ret==ESP_OK);
    }

    for (int x=0; x<3; x++) {
        spi_transaction_t* ptr;
        ret=spi_device_get_trans_result(spi1, &ptr, portMAX_DELAY);
        assert(ret==ESP_OK);
        assert(ptr==spi1_trans+x);
    }
}

void ssd1331_set_dc_line(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(SSD1331_GPIO_PIN_DC, dc);
}

inline void ssd1331_write_byte(unsigned char chData, unsigned char chCmd)
{
	if (chCmd == SSD1331_DATA) {
	 	SSD1331_DC_SET();
	} else {
	 	SSD1331_DC_CLR();
	}

	SSD1331_WRITE_BYTE(chData);
}

void ssd1331_draw_point(unsigned char chXpos, unsigned char chYpos, unsigned int hwColor)
{
	if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
		return;
	}

    ssd1331_write_byte(SET_COLUMN_ADDRESS, SSD1331_CMD);
    ssd1331_write_byte(chXpos, SSD1331_CMD);
    ssd1331_write_byte(chXpos, SSD1331_CMD);

    ssd1331_write_byte(SET_ROW_ADDRESS, SSD1331_CMD);
    ssd1331_write_byte(chYpos, SSD1331_CMD);
    ssd1331_write_byte(chYpos, SSD1331_CMD);
    
	ssd1331_write_byte(hwColor >> 8, SSD1331_DATA);
	ssd1331_write_byte(hwColor, SSD1331_DATA);   
}

void ssd1331_draw_line(unsigned char chXpos0, unsigned char chYpos0, unsigned char chXpos1, unsigned char chYpos1, unsigned int hwColor)
{
	if (chXpos0 >= SSD1331_WIDTH || chYpos0 >= SSD1331_HEIGHT || chXpos1 >= SSD1331_WIDTH || chYpos1 >= SSD1331_HEIGHT) {
		return;
	}
    
    ssd1331_write_byte(DRAW_LINE, SSD1331_CMD);

    ssd1331_write_byte(chXpos0, SSD1331_CMD);
    ssd1331_write_byte(chYpos0, SSD1331_CMD);
    ssd1331_write_byte(chXpos1, SSD1331_CMD);
    ssd1331_write_byte(chYpos1, SSD1331_CMD);

    ssd1331_write_byte(hwColor >> 10, SSD1331_CMD);
    ssd1331_write_byte(hwColor >> 5, SSD1331_CMD);
    ssd1331_write_byte(hwColor << 1, SSD1331_CMD);
}

void ssd1331_draw_v_line(unsigned char chXpos, unsigned char chYpos, unsigned char chHeight, unsigned int hwColor)
{	
	unsigned int y1 = min(chYpos + chHeight, SSD1331_HEIGHT - 1);

	if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
		return;
	}

    ssd1331_write_byte(DRAW_LINE, SSD1331_CMD);

    ssd1331_write_byte(chXpos, SSD1331_CMD);
    ssd1331_write_byte(chYpos, SSD1331_CMD);
    ssd1331_write_byte(chXpos, SSD1331_CMD);
    ssd1331_write_byte(y1, SSD1331_CMD);

    ssd1331_write_byte(hwColor << 1, SSD1331_CMD);
    ssd1331_write_byte(hwColor >> 5, SSD1331_CMD);
    ssd1331_write_byte(hwColor >> 11, SSD1331_CMD);
}

void ssd1331_draw_h_line(unsigned char chXpos, unsigned char chYpos, unsigned char chWidth, unsigned int hwColor)
{	
	unsigned int x1 = min(chXpos + chWidth, SSD1331_WIDTH- 1);

	if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
		return;
	}
	
    ssd1331_write_byte(DRAW_LINE, SSD1331_CMD);

    ssd1331_write_byte(chXpos, SSD1331_CMD);
    ssd1331_write_byte(chYpos, SSD1331_CMD);
    ssd1331_write_byte(x1, SSD1331_CMD);
    ssd1331_write_byte(chYpos, SSD1331_CMD);

    ssd1331_write_byte(hwColor << 1, SSD1331_CMD);
    ssd1331_write_byte(hwColor >> 5, SSD1331_CMD);
    ssd1331_write_byte(hwColor >> 11, SSD1331_CMD);
}

void ssd1331_draw_rect(unsigned char chXpos, unsigned char chYpos, unsigned char chWidth, unsigned char chHeight, unsigned int hwColor)
{
	if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
		return;
	}

    ssd1331_write_byte(DRAW_RECTANGLE, SSD1331_CMD);

    ssd1331_write_byte(chXpos, SSD1331_CMD);
    ssd1331_write_byte(chYpos, SSD1331_CMD);
    ssd1331_write_byte(chXpos + chWidth - 1, SSD1331_CMD);
    ssd1331_write_byte(chYpos + chHeight - 1, SSD1331_CMD);

    ssd1331_write_byte(hwColor >> 10, SSD1331_CMD);
    ssd1331_write_byte(hwColor >> 5, SSD1331_CMD);
    ssd1331_write_byte(hwColor << 1, SSD1331_CMD);

    ssd1331_write_byte(0x00, SSD1331_CMD);
    ssd1331_write_byte(0x00, SSD1331_CMD);
    ssd1331_write_byte(0x00, SSD1331_CMD);
}

void ssd1331_draw_circle(unsigned char chXpos, unsigned char chYpos, unsigned char chRadius, unsigned int hwColor)
{
	int x = -chRadius, y = 0, err = 2 - 2 * chRadius, e2;

	if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
		return;
	}
	
    do {
        ssd1331_draw_point(chXpos - x, chYpos + y, hwColor);
        ssd1331_draw_point(chXpos + x, chYpos + y, hwColor);
        ssd1331_draw_point(chXpos + x, chYpos - y, hwColor);
        ssd1331_draw_point(chXpos - x, chYpos - y, hwColor);
        e2 = err;
        if (e2 <= y) {
            err += ++y * 2 + 1;
            if(-x == y && e2 <= x) e2 = 0;
        }
        if(e2 > x) err += ++x * 2 + 1;
    } while (x <= 0);
}

void ssd1331_draw_mono_bitmap(unsigned char chXpos, unsigned char chYpos, const unsigned char *pchBmp, unsigned char chWidth, unsigned char chHeight, unsigned int hwForeColor, unsigned int hwBackColor)
{
    unsigned char i, j, byteWidth = (chWidth + 7) / 8;

    if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
        return;
    }

    ssd1331_write_byte(SET_COLUMN_ADDRESS, SSD1331_CMD);
    ssd1331_write_byte(chXpos, SSD1331_CMD);
    ssd1331_write_byte(chXpos + chWidth - 1, SSD1331_CMD);

    ssd1331_write_byte(SET_ROW_ADDRESS, SSD1331_CMD);
    ssd1331_write_byte(chYpos, SSD1331_CMD);
    ssd1331_write_byte(chYpos + chHeight - 1, SSD1331_CMD);

    for (i = 0; i < chHeight; i++) {
        for (j = 0; j < chWidth; j++) {
            if(*(pchBmp + j * byteWidth + i / 8) & (128 >> (i & 7))) {
                ssd1331_write_byte(hwForeColor >> 8, SSD1331_DATA);
                ssd1331_write_byte(hwForeColor, SSD1331_DATA);
            }
            else {
                ssd1331_write_byte(hwBackColor >> 8, SSD1331_DATA);
                ssd1331_write_byte(hwBackColor, SSD1331_DATA);
            }
        }
    }
}

void ssd1331_draw_64k_bitmap(unsigned char chXpos, unsigned char chYpos, const unsigned char *pchBmp, unsigned char chWidth, unsigned char chHeight)
{
    unsigned char i, j;

    if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
        return;
    }

    ssd1331_write_byte(SET_COLUMN_ADDRESS, SSD1331_CMD);
    ssd1331_write_byte(chXpos, SSD1331_CMD);
    ssd1331_write_byte(chXpos + chWidth - 1, SSD1331_CMD);

    ssd1331_write_byte(SET_ROW_ADDRESS, SSD1331_CMD);
    ssd1331_write_byte(chYpos, SSD1331_CMD);
    ssd1331_write_byte(chYpos + chHeight - 1, SSD1331_CMD);

    for (i = 0; i < chHeight; i++) {
        for (j = 0; j < chWidth; j++) {
            ssd1331_write_byte(*pchBmp++, SSD1331_DATA);
            ssd1331_write_byte(*pchBmp++, SSD1331_DATA);
        }
    }
}

void ssd1331_fill_rect(unsigned char chXpos, unsigned char chYpos, unsigned char chWidth, unsigned char chHeight, unsigned int hwColor)
{
    if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
        return;
    }

    ssd1331_write_byte(DRAW_RECTANGLE, SSD1331_CMD);

    ssd1331_write_byte(chXpos, SSD1331_CMD);
    ssd1331_write_byte(chYpos, SSD1331_CMD);
    ssd1331_write_byte(chXpos + chWidth - 1, SSD1331_CMD);
    ssd1331_write_byte(chYpos + chHeight - 1, SSD1331_CMD);

    ssd1331_write_byte(hwColor >> 10, SSD1331_CMD);
    ssd1331_write_byte(hwColor >> 5, SSD1331_CMD);
    ssd1331_write_byte(hwColor << 1, SSD1331_CMD);

    ssd1331_write_byte(hwColor >> 10, SSD1331_CMD);
    ssd1331_write_byte(hwColor >> 5, SSD1331_CMD);
    ssd1331_write_byte(hwColor << 1, SSD1331_CMD);
}

void ssd1331_fill_gram(unsigned int hwColor)
{
    ssd1331_fill_rect(0x00, 0x00, SSD1331_WIDTH, SSD1331_HEIGHT, hwColor);
}

void ssd1331_clear_rect(unsigned char chXpos, unsigned char chYpos, unsigned char chWidth, unsigned char chHeight)
{
    if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
        return;
    }

    ssd1331_write_byte(CLEAR_WINDOW, SSD1331_CMD);
    ssd1331_write_byte(chXpos, SSD1331_CMD);
    ssd1331_write_byte(chYpos, SSD1331_CMD);
    ssd1331_write_byte(chXpos + chWidth - 1, SSD1331_CMD);
    ssd1331_write_byte(chYpos + chHeight - 1, SSD1331_CMD);
}

void ssd1331_clear_gram(void)
{
    unsigned char i, j;

    ssd1331_write_byte(SET_COLUMN_ADDRESS, SSD1331_CMD);
    ssd1331_write_byte(0x00, SSD1331_CMD);
    ssd1331_write_byte(SSD1331_WIDTH - 1, SSD1331_CMD);

    ssd1331_write_byte(SET_ROW_ADDRESS, SSD1331_CMD);
    ssd1331_write_byte(0x00, SSD1331_CMD);
    ssd1331_write_byte(SSD1331_HEIGHT - 1, SSD1331_CMD);

    for (i = 0; i < SSD1331_HEIGHT; i++) {
        for (j = 0; j < SSD1331_WIDTH; j++) {
            ssd1331_write_byte(0x00, SSD1331_DATA);
            ssd1331_write_byte(0x00, SSD1331_DATA);
        }
    }
}

void ssd1331_display_char(unsigned char chXpos, unsigned char chYpos, unsigned char chChr, unsigned char chFontIndex, unsigned int hwForeColor, unsigned int hwBackColor)
{
    unsigned char i, j;
    unsigned char chTemp, chYpos0 = chYpos;

    if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
        return;
    }

    for (i = 0; i < fonts_width[chFontIndex] * ((fonts_height[chFontIndex] + 7) / 8); i++) {
        switch (chFontIndex) {
            case FONT_1206:
                chTemp = c_chFont1206[chChr - ' '][i];
                break;
            case FONT_1608:
                chTemp = c_chFont1608[chChr - ' '][i];
                break;
            case FONT_1616:
                chTemp = c_chFont1616[chChr - ' '][i];
                break;
            case FONT_3216:
                chTemp = c_chFont3216[chChr - ' '][i];
                break;
            default:
                chTemp = 0x00;
                break;
        }

        for (j = 0; j < 8; j++) {
            if (chTemp & 0x80) {
                ssd1331_draw_point(chXpos, chYpos, hwForeColor);
            }
            else {
                ssd1331_draw_point(chXpos, chYpos, hwBackColor);
            }
            chTemp <<= 1;
            chYpos++;

            if ((chYpos - chYpos0) == fonts_height[chFontIndex]) {
                chYpos = chYpos0;
                chXpos++;
                break;
            }
        }
    } 
}

static unsigned long _pow(unsigned char m, unsigned char n)
{
    unsigned long result = 1;

    while(n --) result *= m;
    return result;
}


void ssd1331_display_num(unsigned char chXpos, unsigned char chYpos, unsigned long chNum, unsigned char chLen, unsigned char chFontIndex, unsigned int hwForeColor, unsigned int hwBackColor)
{
    unsigned char i;
    unsigned char chTemp, chShow = 0;

    if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
        return;
    }

    for(i = 0; i < chLen; i++) {
        chTemp = (chNum / _pow(10, chLen - i - 1)) % 10;
        if(chShow == 0 && i < (chLen - 1)) {
            if(chTemp == 0) {
                ssd1331_display_char(chXpos + fonts_width[chFontIndex] * i, chYpos, ' ', chFontIndex, hwForeColor, hwBackColor);
                continue;
            } else {
                chShow = 1;
            }
        }
        ssd1331_display_char(chXpos + fonts_width[chFontIndex] * i, chYpos, chTemp + '0', chFontIndex, hwForeColor, hwBackColor);
    }
} 

void ssd1331_display_string(unsigned char chXpos, unsigned char chYpos, const char *pchString, unsigned char chFontIndex, unsigned int hwForeColor, unsigned int hwBackColor)
{
    if (chXpos >= SSD1331_WIDTH || chYpos >= SSD1331_HEIGHT) {
        return;
    }

    while (*pchString != '\0') {       
        if (chXpos > (SSD1331_WIDTH - fonts_width[chFontIndex])) {
            chXpos = 0;
            chYpos += fonts_height[chFontIndex];
            if (chYpos > (SSD1331_HEIGHT - fonts_height[chFontIndex])) {
                chYpos = chXpos = 0;
                ssd1331_clear_gram();
            }
        }

        ssd1331_display_char(chXpos, chYpos, *pchString, chFontIndex, hwForeColor, hwBackColor);
        chXpos += fonts_width[chFontIndex];
        pchString++;
    } 
}

void ssd1331_continuous_scrolling(unsigned char chYpos, unsigned char chHeight, unsigned char chDirection, unsigned char chInterval)
{
    if (chYpos >= SSD1331_WIDTH || (chYpos+chHeight) >= SSD1331_WIDTH) {
        return;
    }

    ssd1331_write_byte(ACTIVATE_SCROLLING, SSD1331_CMD);
}

void ssd1331_deactivate_scrolling(void)
{
    ssd1331_write_byte(DEACTIVATE_SCROLLING, SSD1331_CMD);
}

void ssd1331_show_checkerboard(void)
{
    unsigned char i,j;

    ssd1331_write_byte(SET_COLUMN_ADDRESS, SSD1331_CMD);
    ssd1331_write_byte(0x00, SSD1331_CMD);
    ssd1331_write_byte(SSD1331_WIDTH - 1, SSD1331_CMD);

    ssd1331_write_byte(SET_ROW_ADDRESS, SSD1331_CMD);
    ssd1331_write_byte(0x00, SSD1331_CMD);
    ssd1331_write_byte(SSD1331_HEIGHT - 1, SSD1331_CMD);

    for (i=0; i<SSD1331_HEIGHT/2; i++) {
        for (j=0; j<SSD1331_WIDTH/2; j++) {
            ssd1331_write_byte(0xFF, SSD1331_DATA);
            ssd1331_write_byte(0xFF, SSD1331_DATA);
            ssd1331_write_byte(0x00, SSD1331_DATA);
            ssd1331_write_byte(0x00, SSD1331_DATA);
        }
        for (j=0; j<SSD1331_WIDTH/2; j++) {
            ssd1331_write_byte(0x00, SSD1331_DATA);
            ssd1331_write_byte(0x00, SSD1331_DATA);
            ssd1331_write_byte(0xFF, SSD1331_DATA);
            ssd1331_write_byte(0xFF, SSD1331_DATA);
        }
    }
}

void ssd1331_show_rainbow(void)
{
    // White => Column 0~11
    ssd1331_fill_rect(0x00, 0x00, 0x0C, SSD1331_HEIGHT, White);

    // Yellow => Column 12~23
    ssd1331_fill_rect(0x0C, 0x00, 0x0C, SSD1331_HEIGHT, Red);

    // Purple => Column 24~35
    ssd1331_fill_rect(0x18, 0x00, 0x0C, SSD1331_HEIGHT, Orange);

    // Cyan => Column 36~47
    ssd1331_fill_rect(0x24, 0x00, 0x0C, SSD1331_HEIGHT, Yellow);

    // Red => Column 48~59
    ssd1331_fill_rect(0x30, 0x00, 0x0C, SSD1331_HEIGHT, Lime);

    // Green => Column 60~71
    ssd1331_fill_rect(0x3C, 0x00, 0x0C, SSD1331_HEIGHT, Cyan);

    // Blue => Column 72~83
    ssd1331_fill_rect(0x48, 0x00, 0x0C, SSD1331_HEIGHT, Blue);

    // Black => Column 84~95
    ssd1331_fill_rect(0x54, 0x00, 0x0C, SSD1331_HEIGHT, Magenta);
}

inline void ssd1331_set_gray_scale_table(void)
{
    ssd1331_write_byte(SET_GRAY_SCALE_TABLE, SSD1331_CMD);  // Set Pulse Width for Gray Scale Table
    ssd1331_write_byte(0x02, SSD1331_CMD);                 // Gray Scale Level 1
    ssd1331_write_byte(0x04, SSD1331_CMD);                 // Gray Scale Level 3
    ssd1331_write_byte(0x06, SSD1331_CMD);                 // Gray Scale Level 5
    ssd1331_write_byte(0x08, SSD1331_CMD);                 // Gray Scale Level 7
    ssd1331_write_byte(0x0A, SSD1331_CMD);                 // Gray Scale Level 9
    ssd1331_write_byte(0x0C, SSD1331_CMD);                 // Gray Scale Level 11
    ssd1331_write_byte(0x0E, SSD1331_CMD);                 // Gray Scale Level 13
    ssd1331_write_byte(0x10, SSD1331_CMD);                 // Gray Scale Level 15
    ssd1331_write_byte(0x12, SSD1331_CMD);                 // Gray Scale Level 17
    ssd1331_write_byte(0x15, SSD1331_CMD);                 // Gray Scale Level 19
    ssd1331_write_byte(0x19, SSD1331_CMD);                 // Gray Scale Level 21
    ssd1331_write_byte(0x1D, SSD1331_CMD);                 // Gray Scale Level 23
    ssd1331_write_byte(0x21, SSD1331_CMD);                 // Gray Scale Level 25
    ssd1331_write_byte(0x25, SSD1331_CMD);                 // Gray Scale Level 27
    ssd1331_write_byte(0x2A, SSD1331_CMD);                 // Gray Scale Level 29
    ssd1331_write_byte(0x30, SSD1331_CMD);                 // Gray Scale Level 31
    ssd1331_write_byte(0x36, SSD1331_CMD);                 // Gray Scale Level 33
    ssd1331_write_byte(0x3C, SSD1331_CMD);                 // Gray Scale Level 35
    ssd1331_write_byte(0x42, SSD1331_CMD);                 // Gray Scale Level 37
    ssd1331_write_byte(0x48, SSD1331_CMD);                 // Gray Scale Level 39
    ssd1331_write_byte(0x50, SSD1331_CMD);                 // Gray Scale Level 41
    ssd1331_write_byte(0x58, SSD1331_CMD);                 // Gray Scale Level 43
    ssd1331_write_byte(0x60, SSD1331_CMD);                 // Gray Scale Level 45
    ssd1331_write_byte(0x68, SSD1331_CMD);                 // Gray Scale Level 47
    ssd1331_write_byte(0x70, SSD1331_CMD);                 // Gray Scale Level 49
    ssd1331_write_byte(0x78, SSD1331_CMD);                 // Gray Scale Level 51
    ssd1331_write_byte(0x82, SSD1331_CMD);                 // Gray Scale Level 53
    ssd1331_write_byte(0x8C, SSD1331_CMD);                 // Gray Scale Level 55
    ssd1331_write_byte(0x96, SSD1331_CMD);                 // Gray Scale Level 57
    ssd1331_write_byte(0xA0, SSD1331_CMD);                 // Gray Scale Level 59
    ssd1331_write_byte(0xAA, SSD1331_CMD);                 // Gray Scale Level 61
    ssd1331_write_byte(0xB4, SSD1331_CMD);                 // Gray Scale Level 63
}

void ssd1331_init(void)
{
    SSD1331_PIN_SET();

	ssd1331_write_byte(SET_DISPLAY_OFF, SSD1331_CMD);           // Display Off

	ssd1331_write_byte(SET_REMAP_COLOR_DEPTH, SSD1331_CMD);     // Set Re-Map / Color Depth
    ssd1331_write_byte(0x72, SSD1331_CMD);                      // Set Horizontal Address Increment
    ssd1331_write_byte(SET_DISPLAY_START_LINE, SSD1331_CMD);    // Set Vertical Scroll by RAM
    ssd1331_write_byte(0x00, SSD1331_CMD);                      // Set Mapping RAM Display Start Line
    ssd1331_write_byte(SET_DISPLAY_OFFSET, SSD1331_CMD);        // Set Vertical Scroll by Row
    ssd1331_write_byte(0x00, SSD1331_CMD);                      // Shift Mapping RAM Counter

    ssd1331_write_byte(SET_NORMAL_DISPLAY, SSD1331_CMD);        // Normal Display Mode

    ssd1331_write_byte(SET_MULTIPLEX_RATIO, SSD1331_CMD);       // Set Multiplex Ratio
    ssd1331_write_byte(0x3F, SSD1331_CMD);                      // 1/128 Duty
    ssd1331_write_byte(SET_MASTER_CONFIG, SSD1331_CMD);         // Master Contrast Configuration
    ssd1331_write_byte(0x8E, SSD1331_CMD);                      // Maximum
    ssd1331_write_byte(SET_POWER_SAVE_MODE, SSD1331_CMD);       // Set Power Saving Mode
    ssd1331_write_byte(0x0B, SSD1331_CMD);                      // Disable Power Saving Mode
    ssd1331_write_byte(SET_PHASE_PERIOD_ADJ, SSD1331_CMD);      // Set Reset (Phase1)/Pre-charge (Phase 2) period
    ssd1331_write_byte(0x31, SSD1331_CMD);                      // 0x31
    ssd1331_write_byte(SET_DISPLAY_CLOCK_DIV, SSD1331_CMD);     // Set Display Clock Divider / Oscillator Frequency
    ssd1331_write_byte(0xF0, SSD1331_CMD);                      // 0xF0

    ssd1331_write_byte(SET_PRECHARGE_SPEED_A, SSD1331_CMD);     // Set Second Pre-charge Speed of Color A
    ssd1331_write_byte(0x64, SSD1331_CMD);                      // 100
    ssd1331_write_byte(SET_PRECHARGE_SPEED_B, SSD1331_CMD);     // Set Second Pre-charge Speed of Color B
    ssd1331_write_byte(0x78, SSD1331_CMD);                      // 120
    ssd1331_write_byte(SET_PRECHARGE_SPEED_C, SSD1331_CMD);     // Set Second Pre-charge Speed of Color C
    ssd1331_write_byte(0x64, SSD1331_CMD);                      // 100

    ssd1331_write_byte(SET_PRECHARGE_LEVEL, SSD1331_CMD);       // Set Pre-charge Level
    ssd1331_write_byte(0x3A, SSD1331_CMD);                      // 0x3A
    ssd1331_write_byte(SET_VCOMH_VOLTAGE, SSD1331_CMD);         // Set COM Deselect Voltage Level
    ssd1331_write_byte(0x3E, SSD1331_CMD);                      // 0.83*VCC
    ssd1331_write_byte(SET_MASTER_CURRENT, SSD1331_CMD);        // Master Contrast Current Control
    ssd1331_write_byte(0x0F, SSD1331_CMD);                      // 0x00 ~ 0x0F

    ssd1331_write_byte(SET_CONTRAST_A, SSD1331_CMD);            // Set Contrast Current for Color A
    ssd1331_write_byte(0x91, SSD1331_CMD);                      // 145 0x91
    ssd1331_write_byte(SET_CONTRAST_B, SSD1331_CMD);            // Set Contrast Current for Color B
    ssd1331_write_byte(0x50, SSD1331_CMD);                      // 80 0x50
    ssd1331_write_byte(SET_CONTRAST_C, SSD1331_CMD);            // Set Contrast Current for Color C
    ssd1331_write_byte(0x7D, SSD1331_CMD);                      // 125 0x7D

    ssd1331_write_byte(SET_FILL_MODE, SSD1331_CMD);             // Set Fill Mode
    ssd1331_write_byte(0x01, SSD1331_CMD);                      // Enable Fill for Draw Rectangle

    // ssd1331_write_byte(SET_BUILTIN_LINEAR_LUT, SSD1331_CMD);    // Default
    ssd1331_set_gray_scale_table();                             // Set Pulse Width for Gray Scale Table

    ssd1331_clear_gram();

    ssd1331_write_byte(SET_DISPLAY_ON_NORMAL, SSD1331_CMD);     // Display On
}
