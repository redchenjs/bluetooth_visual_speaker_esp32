/*
 * config.h
 *
 *  Created on: 2019-04-21 17:46
 *      Author: Jack Chen <redchenjs@live.com>
 */

#if defined(CONFIG_PN532_IFCE_UART)
    #define DRIVER_PN532_UART_ENABLED
#elif defined(CONFIG_PN532_IFCE_I2C)
    #define DRIVER_PN532_I2C_ENABLED
#endif

#define PACKAGE_VERSION "libnfc-1.7.1"
