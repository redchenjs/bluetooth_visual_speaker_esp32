/*
 * ssd1351.h
 *
 *  Created on: 2018-03-14 18:04
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_DRIVER_SSD1351_H_
#define INC_DRIVER_SSD1351_H_

#include <stdint.h>

#include "device/spi.h"

extern void ssd1351_init_board(void);
extern void ssd1351_setpin_dc(spi_transaction_t *);
extern void ssd1351_setpin_reset(uint8_t rst);

extern void ssd1351_write_cmd(uint8_t cmd);
extern void ssd1351_write_data(uint8_t data);
extern void ssd1351_refresh_gram(uint8_t *gram);

#endif /* INC_DRIVER_SSD1351_H_ */
