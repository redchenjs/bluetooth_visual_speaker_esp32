/*
 * bt_av.c
 *
 *  Created on: 2019-04-29 12:33
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#include "core/os.h"
#include "core/app.h"

#include "user/led.h"
#include "user/key.h"
#include "user/fft.h"
#include "user/audio_player.h"
#include "user/audio_render.h"

#define BT_A2D_TAG   "bt_a2d"
#define BT_RC_CT_TAG "bt_rc_ct"
#define BT_RC_TG_TAG "bt_rc_tg"

// AVRCP transaction labels
#define APP_RC_CT_TL_GET_CAPS            (0)
#define APP_RC_CT_TL_GET_META_DATA       (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE     (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE  (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE  (4)

esp_bd_addr_t a2d_remote_bda = {0};
unsigned int a2d_sample_rate = 16000;

static const char *s_a2d_conn_state_str[] = {"disconnected", "connecting", "connected", "disconnecting"};
static const char *s_avrc_conn_state_str[] = {"disconnected", "connected"};

void bt_a2d_data_handler(const uint8_t *data, uint32_t len)
{
    if (audio_buff) {
        uint32_t pkt = 0, remain = 0;

        for (pkt = 0; pkt < len / FFT_BLOCK_SIZE; pkt++) {
            xRingbufferSend(audio_buff, data + pkt * FFT_BLOCK_SIZE, FFT_BLOCK_SIZE, portMAX_DELAY);
            taskYIELD();
        }

        remain = len - pkt * FFT_BLOCK_SIZE;
        if (remain != 0) {
            xRingbufferSend(audio_buff, data + pkt * FFT_BLOCK_SIZE, remain, portMAX_DELAY);
            taskYIELD();
        }
    }
}

void bt_a2d_event_handler(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        ESP_LOGI(BT_A2D_TAG, "connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_a2d_conn_state_str[param->conn_stat.state],
                 param->conn_stat.remote_bda[0], param->conn_stat.remote_bda[1],
                 param->conn_stat.remote_bda[2], param->conn_stat.remote_bda[3],
                 param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[5]);

        if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
            memset(&a2d_remote_bda, 0x00, sizeof(esp_bd_addr_t));

            if (!(xEventGroupGetBits(user_event_group) & (OS_PWR_RESET_BIT | OS_PWR_SLEEP_BIT))) {
#ifdef CONFIG_ENABLE_SLEEP_KEY
                key_set_scan_mode(KEY_SCAN_MODE_IDX_ON);
#endif
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            }

#ifdef CONFIG_ENABLE_AUDIO_PROMPT
            audio_player_play_file(MP3_FILE_IDX_DISCONNECTED);
#endif
#ifdef CONFIG_ENABLE_LED
            led_set_mode(LED_MODE_IDX_BLINK_M0);
#endif
            xEventGroupSetBits(user_event_group, BT_A2DP_IDLE_BIT);
        } else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
            memcpy(&a2d_remote_bda, param->conn_stat.remote_bda, sizeof(esp_bd_addr_t));

            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

#ifdef CONFIG_ENABLE_SLEEP_KEY
            if (!(xEventGroupGetBits(user_event_group) & (OS_PWR_RESET_BIT | OS_PWR_SLEEP_BIT))) {
                key_set_scan_mode(KEY_SCAN_MODE_IDX_ON);
            }
#endif
#ifdef CONFIG_ENABLE_AUDIO_PROMPT
            audio_player_play_file(MP3_FILE_IDX_CONNECTED);
#endif
#ifdef CONFIG_ENABLE_LED
            led_set_mode(LED_MODE_IDX_BLINK_M1);
#endif
            xEventGroupClearBits(user_event_group, BT_A2DP_IDLE_BIT);
        } else {
#ifdef CONFIG_ENABLE_SLEEP_KEY
            key_set_scan_mode(KEY_SCAN_MODE_IDX_OFF);
#endif
            xEventGroupClearBits(user_event_group, BT_A2DP_IDLE_BIT);
        }

        break;
    case ESP_A2D_AUDIO_STATE_EVT:
#ifdef CONFIG_ENABLE_LED
        if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED) {
            led_set_mode(LED_MODE_IDX_BLINK_S0);
        } else {
            led_set_mode(LED_MODE_IDX_BLINK_M1);
        }
#endif
        break;
    case ESP_A2D_AUDIO_CFG_EVT:
        if (param->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
            char oct0 = param->audio_cfg.mcc.cie.sbc[0];

            if (oct0 & (0x01 << 6)) {
                a2d_sample_rate = 32000;
            } else if (oct0 & (0x01 << 5)) {
                a2d_sample_rate = 44100;
            } else if (oct0 & (0x01 << 4)) {
                a2d_sample_rate = 48000;
            } else {
                a2d_sample_rate = 16000;
            }

            ESP_LOGI(BT_A2D_TAG, "codec type: SBC, sample rate: %d Hz", a2d_sample_rate);
        }
        break;
    default:
        break;
    }
}

void bt_avrc_ct_event_handler(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch (event) {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
        ESP_LOGI(BT_RC_CT_TAG, "connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_avrc_conn_state_str[param->conn_stat.connected],
                 param->conn_stat.remote_bda[0], param->conn_stat.remote_bda[1],
                 param->conn_stat.remote_bda[2], param->conn_stat.remote_bda[3],
                 param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[5]);

        if (param->conn_stat.connected) {
            esp_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
        }

        break;
    case ESP_AVRC_CT_METADATA_RSP_EVT:
        ESP_LOGI(BT_RC_CT_TAG, "metadata id: 0x%02x, text: %s",
                 param->meta_rsp.attr_id, param->meta_rsp.attr_length ? (char *)param->meta_rsp.attr_text : "");
        break;
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        esp_avrc_ct_send_metadata_cmd(APP_RC_CT_TL_GET_META_DATA, ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST
                                                                | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_GENRE);
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_TRACK_CHANGE, ESP_AVRC_RN_TRACK_CHANGE, 0);
        break;
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
        ESP_LOGI(BT_RC_CT_TAG, "remote features: 0x%04x, TG features: 0x%04x", param->rmt_feats.feat_mask, param->rmt_feats.tg_feat_flag);
        break;
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
        ESP_LOGI(BT_RC_CT_TAG, "rn capabilities: 0x%04x", param->get_rn_caps_rsp.evt_set.bits);

        if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &param->get_rn_caps_rsp.evt_set, ESP_AVRC_RN_TRACK_CHANGE)) {
            esp_avrc_ct_send_metadata_cmd(APP_RC_CT_TL_GET_META_DATA, ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST
                                                                    | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_GENRE);
            esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_TRACK_CHANGE, ESP_AVRC_RN_TRACK_CHANGE, 0);
        }

        break;
    default:
        break;
    }
}

void bt_avrc_tg_event_handler(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
    switch (event) {
    case ESP_AVRC_TG_CONNECTION_STATE_EVT:
        ESP_LOGI(BT_RC_TG_TAG, "connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_avrc_conn_state_str[param->conn_stat.connected],
                 param->conn_stat.remote_bda[0], param->conn_stat.remote_bda[1],
                 param->conn_stat.remote_bda[2], param->conn_stat.remote_bda[3],
                 param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[5]);
        break;
    case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
        ESP_LOGI(BT_RC_TG_TAG, "remote features: 0x%04x, CT features: 0x%04x", param->rmt_feats.feat_mask, param->rmt_feats.ct_feat_flag);
        break;
    default:
        break;
    }
}
