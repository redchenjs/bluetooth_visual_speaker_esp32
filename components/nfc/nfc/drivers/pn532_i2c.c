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
 */

/**
 * @file pn532_i2c.c
 * @brief PN532 driver using I2C bus.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include "pn532_i2c.h"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <nfc/nfc.h>

#include "drivers.h"
#include "nfc-internal.h"
#include "chips/pn53x.h"
#include "chips/pn53x-internal.h"
#include "buses/i2c.h"

#define PN532_I2C_DRIVER_NAME "pn532_i2c"

#define LOG_CATEGORY "libnfc.driver.pn532_i2c"
#define LOG_GROUP    NFC_LOG_GROUP_DRIVER

// I2C address of the PN532 chip.
#define PN532_I2C_ADDR 0x24

/*
 * When sending lots of data, the pn532 occasionally fails to respond in time.
 * Since it happens so rarely, lets try to fix it by re-sending the data. This
 * define allows for fine tuning the number of retries.
 */
#define PN532_SEND_RETRIES 3

// Internal data structs
const struct pn53x_io pn532_i2c_io;

struct pn532_i2c_data {
  uint8_t addr;
  i2c_port_t port;
  volatile bool abort_flag;
};

/* Private Functions Prototypes */
static nfc_device *pn532_i2c_open(const nfc_context *context, const nfc_connstring connstring);
static void pn532_i2c_close(nfc_device *pnd);
static int pn532_i2c_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, int timeout);
static int pn532_i2c_ack(nfc_device *pnd);
static int pn532_i2c_abort_command(nfc_device *pnd);
static int pn532_i2c_wakeup(nfc_device *pnd);
static int pn532_i2c_wait_rdyframe(nfc_device *pnd, int timeout);

#define DRIVER_DATA(pnd) ((struct pn532_i2c_data*)(pnd->driver_data))

static void
pn532_i2c_close(nfc_device *pnd)
{
  pn53x_idle(pnd);

  i2c_close(DRIVER_DATA(pnd)->port);

  pn53x_data_free(pnd);
  nfc_device_free(pnd);
}

static nfc_device *
pn532_i2c_open(const nfc_context *context, const nfc_connstring connstring)
{
  char *port_s;
  nfc_device *pnd;

  int connstring_decode_level = connstring_decode(connstring, PN532_I2C_DRIVER_NAME, NULL, &port_s, NULL);

  switch (connstring_decode_level) {
    case 2:
      break;
    case 1:
      return NULL;
      break;
    case 0:
      return NULL;
  }

  i2c_port_t port;

  if (sscanf(port_s, "i2c%1u", &port) != 1) {
    free(port_s);
    return NULL;
  }

  i2c_open(port, PN532_I2C_ADDR);

  pnd = nfc_device_new(context, connstring);
  if (!pnd) {
    perror("malloc");
    free(port_s);
    i2c_close(port);
    return NULL;
  }
  snprintf(pnd->name, sizeof(pnd->name), "%s:%s", PN532_I2C_DRIVER_NAME, port_s);
  free(port_s);

  pnd->driver_data = malloc(sizeof(struct pn532_i2c_data));
  if (!pnd->driver_data) {
    perror("malloc");
    i2c_close(port);
    nfc_device_free(pnd);
    return NULL;
  }
  DRIVER_DATA(pnd)->port = port;
  DRIVER_DATA(pnd)->addr = PN532_I2C_ADDR;

  // Alloc and init chip's data
  if (pn53x_data_new(pnd, &pn532_i2c_io) == NULL) {
    perror("malloc");
    i2c_close(port);
    nfc_device_free(pnd);
    return NULL;
  }

  // SAMConfiguration command if needed to wakeup the chip and pn53x_SAMConfiguration check if the chip is a PN532
  CHIP_DATA(pnd)->type = PN532;
  // This device starts in LowVBat mode
  CHIP_DATA(pnd)->power_mode = LOWVBAT;

  // empirical tuning
  CHIP_DATA(pnd)->timer_correction = 48;
  pnd->driver = &pn532_i2c_driver;

  DRIVER_DATA(pnd)->abort_flag = false;

  // Check communication using "Diagnose" command, with "Communication test" (0x00)
  if (pn53x_check_communication(pnd) < 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "pn53x_check_communication error");
    pn532_i2c_close(pnd);
    return NULL;
  }

  pn53x_init(pnd);
  return pnd;
}

static int
pn532_i2c_wakeup(nfc_device *pnd)
{
  /* No specific.  PN532 holds SCL during wakeup time  */
  CHIP_DATA(pnd)->power_mode = NORMAL; // PN532 should now be awake
  return NFC_SUCCESS;
}

#define PN532_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)
static int
pn532_i2c_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, int timeout)
{
  int res = 0;
  uint8_t retries;

  // Discard any existing data ?

  switch (CHIP_DATA(pnd)->power_mode) {
    case LOWVBAT: {
      /** PN532C106 wakeup. */
      if ((res = pn532_i2c_wakeup(pnd)) < 0) {
        return res;
      }
      // According to PN532 application note, C106 appendix: to go out Low Vbat mode and enter in normal mode we need to send a SAMConfiguration command
      if ((res = pn532_SAMConfiguration(pnd, PSM_NORMAL, 1000)) < 0) {
        return res;
      }
    }
    break;
    case POWERDOWN: {
      if ((res = pn532_i2c_wakeup(pnd)) < 0) {
        return res;
      }
    }
    break;
    case NORMAL:
      // Nothing to do :)
      break;
  };

  uint8_t  abtFrame[PN532_BUFFER_LEN] = { 0x00, 0x00, 0xff };       // Every packet must start with "00 00 ff"
  size_t szFrame = 0;

  if ((res = pn53x_build_frame(abtFrame, &szFrame, pbtData, szData)) < 0) {
    pnd->last_error = res;
    return pnd->last_error;
  }

  for (retries = PN532_SEND_RETRIES; retries > 0; retries--) {
    res = i2c_send(DRIVER_DATA(pnd)->port, abtFrame, szFrame, timeout);
    if (res >= 0)
      break;

    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Failed to transmit data. Retries left: %d.", retries - 1);
  }

  if (res != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to transmit data. (TX)");
    pnd->last_error = res;
    return pnd->last_error;
  }

  uint8_t abtRxBuf[PN53x_ACK_FRAME__LEN];
  res = pn532_i2c_wait_rdyframe(pnd, timeout);
  if (res == NFC_SUCCESS) {
    res = i2c_receive(DRIVER_DATA(pnd)->port, abtRxBuf, sizeof(abtRxBuf), 0, timeout, 3);
  }
  if (res != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG, "%s", "Unable to read ACK");
    if (res == NFC_EOPABORTED) {
      // Send an ACK frame from host to abort the command.
      pn532_i2c_ack(pnd);
    }
    pnd->last_error = res;
    return pnd->last_error;
  }

  if (pn53x_check_ack_frame(pnd, abtRxBuf, sizeof(abtRxBuf)) == 0) {
    // The PN53x is running the sent command
  } else {
    return pnd->last_error;
  }
  return NFC_SUCCESS;
}

static int
pn532_i2c_wait_rdyframe(nfc_device *pnd, int timeout)
{
  bool done = false;
  int res;

  struct timeval start_tv, cur_tv;
  long long duration;

  // I2C response frame additional status byte
  uint8_t rdy;

  if (timeout > 0) {
    // If a timeout is specified, get current timestamp
    gettimeofday(&start_tv, NULL);
  }

  do {
    res = i2c_receive(DRIVER_DATA(pnd)->port, &rdy, 1, 0, timeout, 1);

    if (DRIVER_DATA(pnd)->abort_flag) {
      // Reset abort flag
      DRIVER_DATA(pnd)->abort_flag = false;

      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG,
              "Wait for a READY frame has been aborted.");
      return NFC_EOPABORTED;
    }

    if (res != NFC_SUCCESS) {
      i2c_receive(DRIVER_DATA(pnd)->port, &rdy, 1, 0, timeout, 3);
      done = true;
    } else {
      if (rdy & 1) {
        done = true;
      } else {
        /* Not ready yet. Check for elapsed timeout. */
        i2c_receive(DRIVER_DATA(pnd)->port, &rdy, 1, 0, timeout, 3);
        if (timeout > 0) {
          gettimeofday(&cur_tv, NULL);
          duration = (cur_tv.tv_sec - start_tv.tv_sec) * 1000000L
                     + (cur_tv.tv_usec - start_tv.tv_usec);

          if (duration / 1000 > timeout) {
            res = NFC_ETIMEOUT;
            done = true;

            log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_DEBUG,
                    "timeout reached with no READY frame.");
          }
        }
      }
    }
  } while (!done);

  return res;
}

static int
pn532_i2c_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, int timeout)
{
  uint8_t  abtRxBuf[5];
  size_t len;
  void *abort_p = NULL;

  abort_p = (void *) & (DRIVER_DATA(pnd)->abort_flag);

  pnd->last_error = pn532_i2c_wait_rdyframe(pnd, timeout);
  if (pnd->last_error == NFC_SUCCESS) {
    pnd->last_error = i2c_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 5, abort_p, timeout, 2);
  } else {
    return pnd->last_error;
  }

  if (abort_p && (NFC_EOPABORTED == pnd->last_error)) {
    pn532_i2c_ack(pnd);
    return NFC_EOPABORTED;
  }

  if (pnd->last_error < 0) {
    goto error;
  }

  const uint8_t pn53x_preamble[3] = { 0x00, 0x00, 0xff };
  if (0 != (memcmp(abtRxBuf, pn53x_preamble, 3))) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Frame preamble+start code mismatch");
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if ((0x01 == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Error frame
    i2c_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 3, 0, timeout, 2);
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Application level error detected");
    pnd->last_error = NFC_EIO;
    goto error;
  } else if ((0xff == abtRxBuf[3]) && (0xff == abtRxBuf[4])) {
    // Extended frame
    pnd->last_error = i2c_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 3, 0, timeout, 2);
    if (pnd->last_error != 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
      goto error;
    }
    // (abtRxBuf[0] << 8) + abtRxBuf[1] (LEN) include TFI + (CC+1)
    len = (abtRxBuf[0] << 8) + abtRxBuf[1] - 2;
    if (((abtRxBuf[0] + abtRxBuf[1] + abtRxBuf[2]) % 256) != 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Length checksum mismatch");
      pnd->last_error = NFC_EIO;
      goto error;
    }
  } else {
    // Normal frame
    if (256 != (abtRxBuf[3] + abtRxBuf[4])) {
      // TODO: Retry
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Length checksum mismatch");
      pnd->last_error = NFC_EIO;
      goto error;
    }

    // abtRxBuf[3] (LEN) include TFI + (CC+1)
    len = abtRxBuf[3] - 2;
  }

  if (len > szDataLen) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to receive data: buffer too small. (szDataLen: %" PRIuPTR ", len: %" PRIuPTR ")", szDataLen, len);
    pnd->last_error = NFC_EIO;
    goto error;
  }

  // TFI + PD0 (CC+1)
  pnd->last_error = i2c_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 2, 0, timeout, 2);
  if (pnd->last_error != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    goto error;
  }

  if (abtRxBuf[0] != 0xD5) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "TFI Mismatch");
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if (abtRxBuf[1] != CHIP_DATA(pnd)->last_command + 1) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Command Code verification failed");
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if (len) {
    pnd->last_error = i2c_receive(DRIVER_DATA(pnd)->port, pbtData, len, 0, timeout, 2);
    if (pnd->last_error != 0) {
      log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
      goto error;
    }
  }

  pnd->last_error = i2c_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 2, 0, timeout, 3);
  if (pnd->last_error != 0) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Unable to receive data. (RX)");
    goto error;
  }

  uint8_t btDCS = (256 - 0xD5);
  btDCS -= CHIP_DATA(pnd)->last_command + 1;
  for (size_t szPos = 0; szPos < len; szPos++) {
    btDCS -= pbtData[szPos];
  }

  if (btDCS != abtRxBuf[0]) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Data checksum mismatch");
    pnd->last_error = NFC_EIO;
    goto error;
  }

  if (0x00 != abtRxBuf[1]) {
    log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "%s", "Frame postamble mismatch");
    pnd->last_error = NFC_EIO;
    goto error;
  }
  // The PN53x command is done and we successfully received the reply
  return len;
error:
  i2c_receive(DRIVER_DATA(pnd)->port, abtRxBuf, 1, 0, timeout, 3);
  return pnd->last_error;
}

int
pn532_i2c_ack(nfc_device *pnd)
{
  return i2c_send(DRIVER_DATA(pnd)->port, pn53x_ack_frame, sizeof(pn53x_ack_frame), 0);
}

static int
pn532_i2c_abort_command(nfc_device *pnd)
{
  if (pnd) {
    DRIVER_DATA(pnd)->abort_flag = true;
  }
  return NFC_SUCCESS;
}

const struct pn53x_io pn532_i2c_io = {
  .send       = pn532_i2c_send,
  .receive    = pn532_i2c_receive,
};

const struct nfc_driver pn532_i2c_driver = {
  .name                             = PN532_I2C_DRIVER_NAME,
  .scan_type                        = INTRUSIVE,
  .scan                             = NULL,
  .open                             = pn532_i2c_open,
  .close                            = pn532_i2c_close,
  .strerror                         = pn53x_strerror,

  .initiator_init                   = pn53x_initiator_init,
  .initiator_init_secure_element    = pn532_initiator_init_secure_element,
  .initiator_select_passive_target  = pn53x_initiator_select_passive_target,
  .initiator_poll_target            = pn53x_initiator_poll_target,
  .initiator_select_dep_target      = pn53x_initiator_select_dep_target,
  .initiator_deselect_target        = pn53x_initiator_deselect_target,
  .initiator_transceive_bytes       = pn53x_initiator_transceive_bytes,
  .initiator_transceive_bits        = pn53x_initiator_transceive_bits,
  .initiator_transceive_bytes_timed = pn53x_initiator_transceive_bytes_timed,
  .initiator_transceive_bits_timed  = pn53x_initiator_transceive_bits_timed,
  .initiator_target_is_present      = pn53x_initiator_target_is_present,

  .target_init           = pn53x_target_init,
  .target_send_bytes     = pn53x_target_send_bytes,
  .target_receive_bytes  = pn53x_target_receive_bytes,
  .target_send_bits      = pn53x_target_send_bits,
  .target_receive_bits   = pn53x_target_receive_bits,

  .device_set_property_bool     = pn53x_set_property_bool,
  .device_set_property_int      = pn53x_set_property_int,
  .get_supported_modulation     = pn53x_get_supported_modulation,
  .get_supported_baud_rate      = pn53x_get_supported_baud_rate,
  .device_get_information_about = pn53x_get_information_about,

  .abort_command  = pn532_i2c_abort_command,
  .idle           = pn53x_idle,
  .powerdown      = pn53x_PowerDown,
};
