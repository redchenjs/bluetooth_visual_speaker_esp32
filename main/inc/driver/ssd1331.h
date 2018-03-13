/*
 * ssd1331.c
 *
 *  Created on: 2018-02-10 15:55
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_DRIVER_SSD1331_H_
#define INC_DRIVER_SSD1331_H_

#include <stdint.h>

#include "device/spi.h"

extern void ssd1331_init_board(void);
extern void ssd1331_setpin_dc(spi_transaction_t *);
extern void ssd1331_setpin_reset(uint8_t rst);

extern void ssd1331_write_cmd(uint8_t cmd);
extern void ssd1331_write_data(uint8_t data);
extern void ssd1331_refresh_gram(uint8_t *gram);

#endif /* INC_DRIVER_SSD1331_H_ */
