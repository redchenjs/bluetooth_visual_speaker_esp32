/*
 * spi.c
 *
 *  Created on: 2018-02-10 16:37
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_DEVICE_SPI_C_
#define INC_DEVICE_SPI_C_

#include "driver/spi_master.h"

extern spi_device_handle_t spi1;
extern spi_transaction_t spi1_t;

extern void spi1_init(void);

#endif /* INC_DEVICE_SPI_C_ */
