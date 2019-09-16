/*
 * nfc_emulator.c
 *
 *  Created on: 2019-09-16 16:21
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <errno.h>
#include <string.h>

#include "esp_log.h"

#include "nfc/nfc.h"
#include "nfc/nfc-emulation.h"

#include "user/nfc_app.h"

#define TAG "nfc_emulator"

/* C-ADPU offsets */
#define CLA  0
#define INS  1
#define P1   2
#define P2   3
#define LC   4
#define DATA 5

#define ISO144434A_RATS 0xE0

#define ISO7816_SELECT         0xA4
#define ISO7816_READ_BINARY    0xB0
#define ISO7816_UPDATE_BINARY  0xD6

typedef struct nfcforum_tag4_ndef_data {
    uint8_t *ndef_file;
    size_t   ndef_file_len;
} nfcforum_tag4_ndef_data_t;

typedef enum {NONE, CC_FILE, NDEF_FILE} file;

typedef struct nfcforum_tag4_state_machine_data {
    file current_file;
} nfcforum_tag4_state_machine_data_t;

static uint8_t ndef_file[] = {
    0x00, 43,
    0xd2, 0x20, 0x08, 0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x76,
    0x6e, 0x64, 0x2e, 0x62, 0x6c, 0x75, 0x65, 0x74, 0x6f, 0x6f, 0x74, 0x68, 0x2e, 0x65, 0x70, 0x2e,
    0x6f, 0x6f, 0x62, 0x08, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static nfcforum_tag4_ndef_data_t nfcforum_tag4_data = {
    .ndef_file = ndef_file,
    .ndef_file_len = 43 + 2,
};

static nfcforum_tag4_state_machine_data_t state_machine_data = {
    .current_file = NONE,
};

static uint8_t nfcforum_capability_container[] = {
    0x00, 0x0F, /* CCLEN 15 bytes */
    0x20,       /* Mapping version 2.0, use option -1 to force v1.0 */
    0x00, 0x54, /* MLe Maximum R-ADPU data size */
    0x00, 0xFF, /* MLc Maximum C-ADPU data size */
    0x04,       /* T field of the NDEF File-Control TLV */
    0x06,       /* L field of the NDEF File-Control TLV */
    /* V field of the NDEF File-Control TLV */
    0xE1, 0x04, /* File identifier */
    0xFF, 0xFE, /* Maximum NDEF Size */
    0x00,       /* NDEF file read access condition */
    0x00,       /* NDEF file write access condition */
};

static nfc_target nt = {
    .nm = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_UNDEFINED,   // Will be updated by nfc_target_init()
    },
    .nti = {
        .nai = {
            .abtAtqa  = {0x00, 0x04},
            .abtUid   = {0x08, 0x00, 0xb0, 0x0b},
            .szUidLen = 4,
            .btSak    = 0x20,
            .abtAts   = {0x75, 0x33, 0x92, 0x03},   // Not used by PN532
            .szAtsLen = 4,
        },
    },
};

static int nfcforum_tag4_io(struct nfc_emulator *emulator, const uint8_t *data_in, const size_t data_in_len, uint8_t *data_out, const size_t data_out_len)
{
    int res = 0;

    nfcforum_tag4_ndef_data_t *ndef_data = (nfcforum_tag4_ndef_data_t *)(emulator->user_data);
    nfcforum_tag4_state_machine_data_t *state_machine_data = (nfcforum_tag4_state_machine_data_t *)(emulator->state_machine->data);

    if (data_in_len == 0) {
        return 0;
    }

    if (data_in_len >= 4) {
        if (data_in[CLA] != 0x00) {
            return -ENOTSUP;
        }

        switch (data_in[INS]) {
        case ISO7816_SELECT:
            switch (data_in[P1]) {
            case 0x00:  // Select by ID
                if ((data_in[P2] | 0x0C) != 0x0C) {
                    return -ENOTSUP;
                }

                const uint8_t ndef_capability_container[] = {0xE1, 0x03};
                const uint8_t ndef_file[] = {0xE1, 0x04};

                if ((data_in[LC] == sizeof(ndef_capability_container)) && (0 == memcmp(ndef_capability_container, data_in + DATA, data_in[LC]))) {
                    memcpy(data_out, "\x90\x00", res = 2);
                    state_machine_data->current_file = CC_FILE;
                } else if ((data_in[LC] == sizeof(ndef_file)) && (0 == memcmp(ndef_file, data_in + DATA, data_in[LC]))) {
                    memcpy(data_out, "\x90\x00", res = 2);
                    state_machine_data->current_file = NDEF_FILE;
                } else {
                    memcpy(data_out, "\x6a\x00", res = 2);
                    state_machine_data->current_file = NONE;
                }

                break;
            case 0x04:  // Select by name
                if (data_in[P2] != 0x00) {
                    return -ENOTSUP;
                }

                const uint8_t ndef_tag_application_name_v2[] = {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01};

                if ((data_in[LC] == sizeof(ndef_tag_application_name_v2)) && (0 == memcmp(ndef_tag_application_name_v2, data_in + DATA, data_in[LC]))) {
                    memcpy(data_out, "\x90\x00", res = 2);
                } else {
                    memcpy(data_out, "\x6a\x82", res = 2);
                }

                break;
            default:
                return -ENOTSUP;
            }

            break;
        case ISO7816_READ_BINARY:
            if ((size_t)(data_in[LC] + 2) > data_out_len) {
                return -ENOSPC;
            }

            switch (state_machine_data->current_file) {
            case NONE:
                memcpy(data_out, "\x6a\x82", res = 2);

                break;
            case CC_FILE:
                memcpy(data_out, nfcforum_capability_container + (data_in[P1] << 8) + data_in[P2], data_in[LC]);
                memcpy(data_out + data_in[LC], "\x90\x00", 2);

                res = data_in[LC] + 2;

                break;
            case NDEF_FILE:
                memcpy(data_out, ndef_data->ndef_file + (data_in[P1] << 8) + data_in[P2], data_in[LC]);
                memcpy(data_out + data_in[LC], "\x90\x00", 2);

                res = data_in[LC] + 2;

                break;
            }

            break;
        case ISO7816_UPDATE_BINARY:
            memcpy(ndef_data->ndef_file + (data_in[P1] << 8) + data_in[P2], data_in + DATA, data_in[LC]);

            if ((data_in[P1] << 8) + data_in[P2] == 0) {
                ndef_data->ndef_file_len = (ndef_data->ndef_file[0] << 8) + ndef_data->ndef_file[1] + 2;
            }

            memcpy(data_out, "\x90\x00", res = 2);

            break;
        default: // Unknown
            ESP_LOGW(TAG, "unknown frame %02X, ignored", data_in[INS]);

            res = -ENOTSUP;

            break;
        }
    } else {
        res = -ENOTSUP;
    }

    return res;
}

static struct nfc_emulation_state_machine state_machine = {
    .io   = nfcforum_tag4_io,
    .data = &state_machine_data,
};

struct nfc_emulator nfc_app_emulator = {
    .target = &nt,
    .state_machine = &state_machine,
    .user_data = &nfcforum_tag4_data,
};

void nfc_emulator_update_bt_addr(const char *addr)
{
    uint16_t addr_idx = (ndef_file[0] << 8 | ndef_file[1]) - 6 + 2;

    ndef_file[addr_idx + 5] = addr[0];
    ndef_file[addr_idx + 4] = addr[1];
    ndef_file[addr_idx + 3] = addr[2];
    ndef_file[addr_idx + 2] = addr[3];
    ndef_file[addr_idx + 1] = addr[4];
    ndef_file[addr_idx + 0] = addr[5];
}
