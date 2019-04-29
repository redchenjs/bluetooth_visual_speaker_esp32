/*
 * ST7735.h
 *
 *  Created on: 2019-04-29 22:04
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef _ST7735_H
#define _ST7735_H

// System Function Command List
#define ST7735_NOP       0x00
#define ST7735_SWRESET   0x01
#define ST7735_RDDID     0x04
#define ST7735_RDDST     0x09
#define ST7735_RDDPM     0x0A
#define ST7735_RDDMADCTL 0x0B
#define ST7735_RDDCOLMOD 0x0C
#define ST7735_RDDIM     0x0D
#define ST7735_RDDSM     0x0E
#define ST7735_RDDSDR    0x0F

#define ST7735_SLPIN     0x10
#define ST7735_SLPOUT    0x11
#define ST7735_PTLON     0x12
#define ST7735_NORON     0x13

#define ST7735_INVOFF    0x20
#define ST7735_INVON     0x21
#define ST7735_GAMSET    0x26
#define ST7735_DISPOFF   0x28
#define ST7735_DISPON    0x29
#define ST7735_CASET     0x2A
#define ST7735_RASET     0x2B
#define ST7735_RAMWR     0x2C
#define ST7735_RGBSET    0x2D
#define ST7735_RAMRD     0x2E

#define ST7735_PTLAR     0x30
#define ST7735_SCRLAR    0x33
#define ST7735_TEOFF     0x34
#define ST7735_TEON      0x35
#define ST7735_MADCTL    0x36
#define ST7735_VSCSAD    0x37
#define ST7735_IDMOFF    0x38
#define ST7735_IDMON     0x39
#define ST7735_COLMOD    0x3A

#define ST7735_RDID1     0xDA
#define ST7735_RDID2     0xDB
#define ST7735_RDID3     0xDC

// Panel Function Command List
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4

#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_VMOFCTR  0xC7

#define ST7735_WRID2    0xD1
#define ST7735_WRID3    0xD2
#define ST7735_NVCTR1   0xD9
#define ST7735_NVCTR2   0xDE
#define ST7735_NVCTR3   0xDF

#define ST7735_GAMCTRP1 0xE0
#define ST7735_GAMCTRN1 0xE1

#endif  // _ST7735_H
