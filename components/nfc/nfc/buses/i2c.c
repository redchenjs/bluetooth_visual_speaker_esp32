/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tarti?re
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 * Copyright (C) 2013      Laurent Latil
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/**
 * @file i2c.c
 * @brief I2C driver
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H
#include "i2c.h"

#include "nfc/nfc.h"
#include "nfc-internal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#include "chip/i2c.h"

#define LOG_GROUP    NFC_LOG_GROUP_COM
#define LOG_CATEGORY "libnfc.bus.i2c"

static portTickType xLastWakeTime;

/*
 * Bus free time (in ms) between a STOP condition and START condition. See
 * tBuf in the PN532 data sheet, section 12.25: Timing for the I2C interface,
 * table 320. I2C timing specification, page 211, rev. 3.6 - 2017-11-28.
 */
#define PN532_BUS_FREE_TIME 2

static uint8_t i2c_dev_addr = 0x00;

void
i2c_open(i2c_port_t port, uint8_t addr)
{
  i2c_dev_addr = addr;
  xLastWakeTime = xTaskGetTickCount();
}

void
i2c_close(i2c_port_t port)
{

}

int
i2c_receive(i2c_port_t port, uint8_t *pbtRx, const size_t szRx, void *abort_p, int timeout, uint8_t mode)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  if (mode == 0 || mode == 1) {
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, i2c_dev_addr << 1 | I2C_MASTER_READ, 1);
  }
  if (mode == 1 || mode == 2) {
    i2c_master_read(cmd, pbtRx, szRx, 0);
  }
  if (mode == 0 || mode == 3) {
    if (szRx > 1) {
      i2c_master_read(cmd, pbtRx, szRx - 1, 0);
    }
    i2c_master_read_byte(cmd, pbtRx + szRx - 1, 1);
    i2c_master_stop(cmd);
  }

  if (mode == 0 || mode == 1) {
    vTaskDelayUntil(&xLastWakeTime, PN532_BUS_FREE_TIME / portTICK_PERIOD_MS);
  }
  int res = i2c_master_cmd_begin(port, cmd, timeout / portTICK_RATE_MS);
  if (mode == 0 || mode == 3) {
    xLastWakeTime = xTaskGetTickCount();
  }

  i2c_cmd_link_delete(cmd);

  LOG_HEX(LOG_GROUP, "RX", pbtRx, szRx);
  if (res == ESP_OK) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG,
            "read %d bytes successfully.", szRx);
    return NFC_SUCCESS;
  } else if (res == ESP_ERR_TIMEOUT) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Error: read timeout.");
    return NFC_ETIMEOUT;
  } else {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Error: read error.");
    return NFC_EIO;
  }
}

int
i2c_send(i2c_port_t port, const uint8_t *pbtTx, const size_t szTx, int timeout)
{
  LOG_HEX(LOG_GROUP, "TX", pbtTx, szTx);

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, i2c_dev_addr << 1 | I2C_MASTER_WRITE, 1);
  i2c_master_write(cmd, (uint8_t *)pbtTx, szTx, 1);
  i2c_master_stop(cmd);

  vTaskDelayUntil(&xLastWakeTime, PN532_BUS_FREE_TIME / portTICK_PERIOD_MS);
  int res = i2c_master_cmd_begin(port, cmd, timeout / portTICK_RATE_MS);
  xLastWakeTime = xTaskGetTickCount();

  i2c_cmd_link_delete(cmd);

  if (res == ESP_OK) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG,
            "wrote %d bytes successfully.", szTx);
    return NFC_SUCCESS;
  } else if (res == ESP_ERR_TIMEOUT) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Error: write timeout.");
    return NFC_ETIMEOUT;
  } else {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Error: write error.");
    return NFC_EIO;
  }
}
