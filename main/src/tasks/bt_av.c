/*
 * bt_av.c
 *
 *  Created on: 2018-03-09 13:56
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"

#include "tasks/bt_av.h"
#include "tasks/bt_speaker.h"
#include "tasks/led_indicator.h"

#define TAG "bt_av"

static void bt_av_alloc_meta_buffer(esp_avrc_ct_cb_param_t *param)
{
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(param);
    uint8_t *attr_text = (uint8_t *) malloc (rc->meta_rsp.attr_length + 1);
    memcpy(attr_text, rc->meta_rsp.attr_text, rc->meta_rsp.attr_length);
    attr_text[rc->meta_rsp.attr_length] = 0;

    rc->meta_rsp.attr_text = attr_text;
}

/* callback for A2DP sink */
void bt_av_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
    case ESP_A2D_AUDIO_STATE_EVT:
    case ESP_A2D_AUDIO_CFG_EVT:
        bt_speaker_work_dispatch(bt_av_hdl_a2d_evt, event, param, sizeof(esp_a2d_cb_param_t), NULL);
        break;
    default:
        ESP_LOGE(TAG, "invalid a2dp_cb event: %d", event);
        break;
    }
}

/* cb with decoded samples */
void bt_av_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    i2s_write_bytes(0, (const char *)data, len, portMAX_DELAY);
}

void bt_av_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch (event) {
    case ESP_AVRC_CT_METADATA_RSP_EVT:
        bt_av_alloc_meta_buffer(param);
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
        bt_speaker_work_dispatch(bt_av_hdl_avrc_evt, event, param, sizeof(esp_avrc_ct_cb_param_t), NULL);
        break;
    }
    default:
        ESP_LOGE(TAG, "invalid avrc_cb event: %d", event);
        break;
    }
}

static void bt_av_new_track(void)
{
    //Register notifications and request metadata
    esp_avrc_ct_send_metadata_cmd(0, ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_GENRE);
    esp_avrc_ct_send_register_notification_cmd(1, ESP_AVRC_RN_TRACK_CHANGE, 0);
}

static void bt_av_notify_evt_handler(uint8_t event_id, uint32_t event_parameter)
{
    switch (event_id) {
    case ESP_AVRC_RN_TRACK_CHANGE:
        bt_av_new_track();
        break;
    }
}

void bt_av_hdl_stack_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(TAG, "%s evt %d", __func__, event);
    switch (event) {
    case BT_AV_EVT_STACK_UP:
        ESP_LOGI(TAG, "starting \"%s\"", CONFIG_BT_NAME);
        /* set up device name */
        esp_bt_dev_set_device_name(CONFIG_BT_NAME);

        /* initialize A2DP sink */
        esp_a2d_register_callback(&bt_av_a2d_cb);
        esp_a2d_sink_register_data_callback(bt_av_a2d_data_cb);
        esp_a2d_sink_init();

        /* initialize AVRCP controller */
        esp_avrc_ct_init();
        esp_avrc_ct_register_callback(bt_av_rc_ct_cb);

        /* set discoverable and connectable mode, wait to be connected */
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        break;
    default:
        ESP_LOGE(TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}

void bt_av_hdl_a2d_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(TAG, "%s evt %d", __func__, event);
    esp_a2d_cb_param_t *a2d = NULL;
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        a2d = (esp_a2d_cb_param_t *)(p_param);
        uint8_t *bda = a2d->conn_stat.remote_bda;
        ESP_LOGI(TAG, "a2dp conn_stat evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
             a2d->conn_stat.state, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
        if (ESP_A2D_CONNECTION_STATE_CONNECTED == a2d->conn_stat.state) {
            led_indicator_set_mode(3);
        } else if (ESP_A2D_CONNECTION_STATE_DISCONNECTED == a2d->conn_stat.state) {
            led_indicator_set_mode(5);
        }
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        a2d = (esp_a2d_cb_param_t *)(p_param);
        ESP_LOGI(TAG, "a2dp audio_stat evt: state %d", a2d->audio_stat.state);
        if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state) {
            led_indicator_set_mode(1);
        } else if (ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND == a2d->audio_stat.state){
            led_indicator_set_mode(2);
        } else if (ESP_A2D_AUDIO_STATE_STOPPED == a2d->audio_stat.state){
            led_indicator_set_mode(3);
        }
        break;
    case ESP_A2D_AUDIO_CFG_EVT:
        a2d = (esp_a2d_cb_param_t *)(p_param);
        ESP_LOGI(TAG, "a2dp audio stream configuration, codec type %d", a2d->audio_cfg.mcc.type);
        // for now only SBC stream is supported
        if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
            int sample_rate = 16000;
            char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];
            if (oct0 & (0x01 << 6)) {
                sample_rate = 32000;
            } else if (oct0 & (0x01 << 5)) {
                sample_rate = 44100;
            } else if (oct0 & (0x01 << 4)) {
                sample_rate = 48000;
            }
            i2s_set_clk(0, sample_rate, 16, 2);

            ESP_LOGI(TAG, "configure audio player %x-%x-%x-%x",
                     a2d->audio_cfg.mcc.cie.sbc[0],
                     a2d->audio_cfg.mcc.cie.sbc[1],
                     a2d->audio_cfg.mcc.cie.sbc[2],
                     a2d->audio_cfg.mcc.cie.sbc[3]);
            ESP_LOGI(TAG, "audio player configured, sample rate=%d", sample_rate);
        }
        break;
    default:
        ESP_LOGE(TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}

void bt_av_hdl_avrc_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(TAG, "%s evt %d", __func__, event);
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);
    uint8_t *bda = NULL;
    switch (event) {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
        bda = rc->conn_stat.remote_bda;
        ESP_LOGI(TAG, "avrc conn_stat evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

        if (rc->conn_stat.connected) {
            bt_av_new_track();
        }
        break;
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
        ESP_LOGI(TAG, "avrc passthrough rsp: key_code 0x%x, key_state %d", rc->psth_rsp.key_code, rc->psth_rsp.key_state);
        break;
    case ESP_AVRC_CT_METADATA_RSP_EVT:
        ESP_LOGI(TAG, "avrc metadata rsp: attribute id 0x%x, %s", rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
        free(rc->meta_rsp.attr_text);
        break;
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        ESP_LOGI(TAG, "avrc event notification: %d, param: %d", rc->change_ntf.event_id, rc->change_ntf.event_parameter);
        bt_av_notify_evt_handler(rc->change_ntf.event_id, rc->change_ntf.event_parameter);
        break;
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
        ESP_LOGI(TAG, "avrc remote features %x", rc->rmt_feats.feat_mask);
        break;
    default:
        ESP_LOGE(TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}
