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
#include "user/bt_app.h"
#include "user/bt_app_core.h"
#include "user/audio_player.h"
#include "user/audio_render.h"

#define BT_A2D_TAG   "bt_a2d"
#define BT_RC_CT_TAG "bt_rc_ct"
#define BT_RC_RN_TAG "bt_rc_rn"

// AVRCP transaction labels
#define APP_RC_CT_TL_GET_CAPS            (0)
#define APP_RC_CT_TL_GET_META_DATA       (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE     (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE  (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE  (4)

esp_bd_addr_t a2d_remote_bda = {0};
unsigned int a2d_sample_rate = 16000;

static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap = {0};

static const char *s_a2d_conn_state_str[] = {"disconnected", "connecting", "connected", "disconnecting"};
static const char *s_a2d_audio_state_str[] = {"suspended", "stopped", "started"};
static const char *s_avrc_conn_state_str[] = {"disconnected", "connected"};

static void bt_av_hdl_a2d_evt(uint16_t event, void *p_param)
{
    esp_a2d_cb_param_t *param = (esp_a2d_cb_param_t *)p_param;

    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        ESP_LOGI(BT_A2D_TAG, "A2DP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_a2d_conn_state_str[param->conn_stat.state],
                 param->conn_stat.remote_bda[0], param->conn_stat.remote_bda[1],
                 param->conn_stat.remote_bda[2], param->conn_stat.remote_bda[3],
                 param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[5]);

        if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
            memset(&a2d_remote_bda, 0x00, sizeof(esp_bd_addr_t));

            EventBits_t uxBits = xEventGroupGetBits(user_event_group);
            if (!(uxBits & OS_PWR_RESET_BIT) && !(uxBits & OS_PWR_SLEEP_BIT)) {
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
            xEventGroupClearBits(user_event_group, BT_A2DP_IDLE_BIT);

            memcpy(&a2d_remote_bda, param->conn_stat.remote_bda, sizeof(esp_bd_addr_t));

            if (memcmp(&last_remote_bda, &a2d_remote_bda, sizeof(esp_bd_addr_t)) != 0) {
                memcpy(&last_remote_bda, &a2d_remote_bda, sizeof(esp_bd_addr_t));
                app_setenv("LAST_REMOTE_BDA", &last_remote_bda, sizeof(esp_bd_addr_t));
            }

            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

#ifdef CONFIG_ENABLE_SLEEP_KEY
            EventBits_t uxBits = xEventGroupGetBits(user_event_group);
            if (!(uxBits & OS_PWR_RESET_BIT) && !(uxBits & OS_PWR_SLEEP_BIT)) {
                key_set_scan_mode(KEY_SCAN_MODE_IDX_ON);
            }
#endif
#ifdef CONFIG_ENABLE_AUDIO_PROMPT
            audio_player_play_file(MP3_FILE_IDX_CONNECTED);
#endif
#ifdef CONFIG_ENABLE_LED
            led_set_mode(LED_MODE_IDX_BLINK_M1);
#endif
        } else {
#ifdef CONFIG_ENABLE_SLEEP_KEY
            key_set_scan_mode(KEY_SCAN_MODE_IDX_OFF);
#endif
            xEventGroupClearBits(user_event_group, BT_A2DP_IDLE_BIT);
        }
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        ESP_LOGI(BT_A2D_TAG, "A2DP audio state: %s", s_a2d_audio_state_str[param->audio_stat.state]);

#ifdef CONFIG_ENABLE_LED
        if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED) {
            led_set_mode(LED_MODE_IDX_BLINK_S0);
        } else {
            led_set_mode(LED_MODE_IDX_BLINK_M1);
        }
#endif
        break;
    case ESP_A2D_AUDIO_CFG_EVT:
        ESP_LOGI(BT_A2D_TAG, "A2DP audio stream configuration, codec type: %d", param->audio_cfg.mcc.type);
        // for now only SBC stream is supported
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

            ESP_LOGI(BT_A2D_TAG, "A2DP audio player configuration, sample rate: %d Hz", a2d_sample_rate);
        }

        break;
    default:
        ESP_LOGW(BT_A2D_TAG, "unhandled evt: %d", event);
        break;
    }
}

static void bt_av_new_track(void)
{
    uint8_t attr_mask = ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM;

    // request metadata
    esp_avrc_ct_send_metadata_cmd(APP_RC_CT_TL_GET_META_DATA, attr_mask);

    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap, ESP_AVRC_RN_TRACK_CHANGE)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_TRACK_CHANGE, ESP_AVRC_RN_TRACK_CHANGE, 0);
    }
}

static void bt_av_play_status_changed(void)
{
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap, ESP_AVRC_RN_PLAY_STATUS_CHANGE)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAYBACK_CHANGE, ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
    }
}

static void bt_av_play_pos_changed(void)
{
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap, ESP_AVRC_RN_PLAY_POS_CHANGED)) {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAY_POS_CHANGE, ESP_AVRC_RN_PLAY_POS_CHANGED, 10);
    }
}

static void bt_av_notify_evt_handler(uint8_t event, esp_avrc_rn_param_t *param)
{
    switch (event) {
    case ESP_AVRC_RN_TRACK_CHANGE:
        bt_av_new_track();
        break;
    case ESP_AVRC_RN_PLAY_STATUS_CHANGE:
        ESP_LOGI(BT_RC_RN_TAG, "play status changed: %d", param->playback);
        bt_av_play_status_changed();
        break;
    case ESP_AVRC_RN_PLAY_POS_CHANGED:
        ESP_LOGI(BT_RC_RN_TAG, "play position changed: %d ms", param->play_pos);
        bt_av_play_pos_changed();
        break;
    }
}

static void bt_av_hdl_avrc_ct_evt(uint16_t event, void *p_param)
{
    esp_avrc_ct_cb_param_t *param = (esp_avrc_ct_cb_param_t *)p_param;

    switch (event) {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
        ESP_LOGI(BT_RC_CT_TAG, "AVRC connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_avrc_conn_state_str[param->conn_stat.connected],
                 param->conn_stat.remote_bda[0], param->conn_stat.remote_bda[1],
                 param->conn_stat.remote_bda[2], param->conn_stat.remote_bda[3],
                 param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[5]);

        if (param->conn_stat.connected) {
            // get remote supported event_ids of peer AVRCP Target
            esp_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
        } else {
            // clear peer notification capability record
            s_avrc_peer_rn_cap.bits = 0;
        }
        break;
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
        ESP_LOGI(BT_RC_CT_TAG, "AVRC passthrough rsp, key_code: 0x%x, key_state: %d", param->psth_rsp.key_code, param->psth_rsp.key_state);
        break;
    case ESP_AVRC_CT_METADATA_RSP_EVT:
        ESP_LOGI(BT_RC_CT_TAG, "AVRC metadata rsp, attr_id: 0x%x, attr_text: %s", param->meta_rsp.attr_id, param->meta_rsp.attr_text);
        free(param->meta_rsp.attr_text);
        break;
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        ESP_LOGI(BT_RC_CT_TAG, "AVRC event notification: %d", param->change_ntf.event_id);
        bt_av_notify_evt_handler(param->change_ntf.event_id, &param->change_ntf.event_parameter);
        break;
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
        ESP_LOGI(BT_RC_CT_TAG, "AVRC remote features: %x, TG features: %x", param->rmt_feats.feat_mask, param->rmt_feats.tg_feat_flag);
        break;
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
        ESP_LOGI(BT_RC_CT_TAG, "AVRC rn capabilities: %d, bitmask: 0x%x", param->get_rn_caps_rsp.cap_count, param->get_rn_caps_rsp.evt_set.bits);
        s_avrc_peer_rn_cap.bits = param->get_rn_caps_rsp.evt_set.bits;
        bt_av_new_track();
        bt_av_play_status_changed();
        bt_av_play_pos_changed();
        break;
    default:
        ESP_LOGW(BT_RC_CT_TAG, "unhandled evt: %d", event);
        break;
    }
}

static void bt_app_alloc_meta_buffer(esp_avrc_ct_cb_param_t *param)
{
    uint8_t *attr_text = calloc(param->meta_rsp.attr_length + 1, sizeof(uint8_t));

    memcpy(attr_text, param->meta_rsp.attr_text, param->meta_rsp.attr_length);

    param->meta_rsp.attr_text = attr_text;
}

void bt_app_avrc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch (event) {
    case ESP_AVRC_CT_METADATA_RSP_EVT:
        bt_app_alloc_meta_buffer(param);
        /* fall through */
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
        bt_app_work_dispatch(bt_av_hdl_avrc_ct_evt, event, param, sizeof(esp_avrc_ct_cb_param_t), NULL);
        break;
    default:
        break;
    }
}

void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
    case ESP_A2D_AUDIO_STATE_EVT:
    case ESP_A2D_AUDIO_CFG_EVT:
        bt_app_work_dispatch(bt_av_hdl_a2d_evt, event, param, sizeof(esp_a2d_cb_param_t), NULL);
        break;
    default:
        break;
    }
}

void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
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
