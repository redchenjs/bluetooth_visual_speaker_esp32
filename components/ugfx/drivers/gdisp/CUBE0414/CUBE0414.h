#ifndef _CUBE0414_H
#define _CUBE0414_H

#ifdef CONFIG_CUBE0414_RTL_REV_3
    #define CUBE0414_CONF_WR 0x2A
    #define CUBE0414_ADDR_WR 0x2B
    #define CUBE0414_DATA_WR 0x2C
#else
    #define CUBE0414_ADDR_WR 0xCC
    #define CUBE0414_DATA_WR 0xDA
#endif

#endif
