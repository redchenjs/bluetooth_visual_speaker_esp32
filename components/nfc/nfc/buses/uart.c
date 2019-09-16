/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
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
 * @file uart.c
 * @brief UART driver
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "uart.h"

#include "nfc/nfc.h"
#include "nfc-internal.h"

#include "chip/uart.h"

#define LOG_GROUP    NFC_LOG_GROUP_COM
#define LOG_CATEGORY "libnfc.bus.uart"

void
uart_open(uart_port_t port)
{

}

void
uart_set_speed(uart_port_t port, const uint32_t uiPortSpeed)
{
  log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "Serial port speed requested to be set to %d baud.", uiPortSpeed);

  // Set port speed
  if (uart_set_baudrate(port, uiPortSpeed) != ESP_OK) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to apply new speed settings.");
  }
}

uint32_t
uart_get_speed(uart_port_t port)
{
  uint32_t uiPortSpeed = 0;

  uart_get_baudrate(port, &uiPortSpeed);

  return uiPortSpeed;
}

void
uart_close(const uart_port_t port)
{

}

int
uart_receive(uart_port_t port, uint8_t *pbtRx, const size_t szRx, void *abort_p, int timeout)
{
  int res = uart_read_bytes(port, pbtRx, szRx, timeout / portTICK_RATE_MS);
  LOG_HEX(LOG_GROUP, "RX", pbtRx, szRx);
  if (res == szRx)
    return NFC_SUCCESS;
  else
    return NFC_EIO;
}

int
uart_send(uart_port_t port, const uint8_t *pbtTx, const size_t szTx, int timeout)
{
  (void) timeout;
  LOG_HEX(LOG_GROUP, "TX", pbtTx, szTx);
  int res = uart_write_bytes(port, (const char *)pbtTx, szTx);
  if (res == szTx)
    return NFC_SUCCESS;
  else
    return NFC_EIO;
}
