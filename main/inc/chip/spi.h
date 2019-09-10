/*
 * spi.h
 *
 *  Created on: 2018-02-10 16:37
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_CHIP_SPI_H_
#define INC_CHIP_SPI_H_

#include "driver/spi_master.h"

extern spi_device_handle_t hspi;

extern void hspi_init(void);

#endif /* INC_CHIP_SPI_H_ */
