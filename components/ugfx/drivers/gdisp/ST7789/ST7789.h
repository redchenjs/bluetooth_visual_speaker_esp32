/*
 * ST7789.h
 *
 *  Created on: 2019-04-29 22:04
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef _ST7789_H
#define _ST7789_H

// System Function Command Table 1
#define ST7789_NOP       0x00
#define ST7789_SWRESET   0x01
#define ST7789_RDDID     0x04
#define ST7789_RDDST     0x09
#define ST7789_RDDPM     0x0A
#define ST7789_RDDMADCTL 0x0B
#define ST7789_RDDCOLMOD 0x0C
#define ST7789_RDDIM     0x0D
#define ST7789_RDDSM     0x0E
#define ST7789_RDDSDR    0x0F

#define ST7789_SLPIN     0x10
#define ST7789_SLPOUT    0x11
#define ST7789_PTLON     0x12
#define ST7789_NORON     0x13

#define ST7789_INVOFF    0x20
#define ST7789_INVON     0x21
#define ST7789_GAMSET    0x26
#define ST7789_DISPOFF   0x28
#define ST7789_DISPON    0x29
#define ST7789_CASET     0x2A
#define ST7789_RASET     0x2B
#define ST7789_RAMWR     0x2C
#define ST7789_RAMRD     0x2E

#define ST7789_PTLAR     0x30
#define ST7789_VSCRDEF   0x33
#define ST7789_TEOFF     0x34
#define ST7789_TEON      0x35
#define ST7789_MADCTL    0x36
#define ST7789_VSCRSADD  0x37
#define ST7789_IDMOFF    0x38
#define ST7789_IDMON     0x39
#define ST7789_COLMOD    0x3A
#define ST7789_RAMWRC    0x3C
#define ST7789_RAMRDC    0x3E

#define ST7789_TESCAN    0x44
#define ST7789_RDTESCAN  0x45

#define ST7789_WRDISBV   0x51
#define ST7789_RDDISBV   0x52
#define ST7789_WRCTRLD   0x53
#define ST7789_RDCTRLD   0x54
#define ST7789_WRCACE    0x55
#define ST7789_RDCABC    0x56
#define ST7789_WRCABCMB  0x5E
#define ST7789_RDCABCMB  0x5F

#define ST7789_RDABCSDR  0x68

#define ST7789_RDID1     0xDA
#define ST7789_RDID2     0xDB
#define ST7789_RDID3     0xDC

// System Function Command Table 2
#define ST7789_RAMCTRL    0xB0
#define ST7789_RGBCTRL    0xB1
#define ST7789_PORCTRL    0xB2
#define ST7789_FRCTRL1    0xB3
#define ST7789_PARCTRL    0xB5
#define ST7789_GCTRL      0xB7
#define ST7789_GTADJ      0xB8
#define ST7789_DGMEN      0xBA
#define ST7789_VCOMS      0xBB
#define ST7789_POWSAVE    0xBC
#define ST7789_DLPOFFSAVE 0xBD

#define ST7789_LCMCTRL    0xC0
#define ST7789_IDSET      0xC1
#define ST7789_VDVVRHEN   0xC2
#define ST7789_VRHS       0xC3
#define ST7789_VDVSET     0xC4
#define ST7789_VCMOFSET   0xC5
#define ST7789_FRCTRL2    0xC6
#define ST7789_CABCCTRL   0xC7
#define ST7789_REGSEL1    0xC8
#define ST7789_REGSEL2    0xCA
#define ST7789_PWMFRSEL   0xCC

#define ST7789_PWCTRL1    0xD0
#define ST7789_VAPVANEN   0xD2
#define ST7789_CMD2EN     0xDF

#define ST7789_PVGAMCTRL  0xE0
#define ST7789_NVGAMCTRL  0xE1
#define ST7789_DGMLUTR    0xE2
#define ST7789_DGMLUTB    0xE3
#define ST7789_GATECTRL   0xE4
#define ST7789_SPI2EN     0xE7
#define ST7789_PWCTRL2    0xE8
#define ST7789_EQCTRL     0xE9
#define ST7789_PROMCTRL   0xEC

#define ST7789_PROMEN     0xFA
#define ST7789_NVMSET     0xFC
#define ST7789_PROMACT    0xFE

#endif  // _ST7789_H
