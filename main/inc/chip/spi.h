/*
 * spi.h
 *
 *  Created on: 2018-02-10 16:37
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_CHIP_SPI_H_
#define INC_CHIP_SPI_H_

#include "driver/spi_master.h"

#define SPI_HOST_TAG "spi-2"
#define SPI_HOST_NUM SPI2_HOST

extern spi_device_handle_t spi_host;

extern void spi_host_init(void);

#endif /* INC_CHIP_SPI_H_ */
