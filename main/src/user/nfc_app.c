/*
 * nfc_app.c
 *
 *  Created on: 2018-02-13 21:50
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"
#include "esp_system.h"

#include "nfc/nfc.h"
#include "nfc/nfc-emulation.h"

#include "core/os.h"
#include "chip/bt.h"
#include "board/pn532.h"
#include "user/nfc_app.h"
#include "user/nfc_emulator.h"

#define TAG "nfc_app"

static nfc_device *pnd = NULL;
static nfc_context *context = NULL;

static TaskHandle_t nfc_app_task_handle = NULL;

static void nfc_app_task(void *pvParameter)
{
    pn532_setpin_reset(1);
    vTaskDelay(100 / portTICK_RATE_MS);

    nfc_init(&context);
    if (context == NULL) {
        ESP_LOGE(TAG, "unable to init libnfc (malloc)");
        goto err;
    }

    nfc_emulator_update_bt_addr(bt_dev_address);

    ESP_LOGI(TAG, "started.");

    while (1) {
        xEventGroupWaitBits(
            user_event_group,
            NFC_APP_RUN_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );
        // Open NFC device
#ifdef CONFIG_PN532_IFCE_UART
        while ((pnd = nfc_open(context, "pn532_uart:uart1:115200")) == NULL) {
#else
        while ((pnd = nfc_open(context, "pn532_i2c:i2c0")) == NULL) {
#endif
            ESP_LOGE(TAG, "device reset");
            pn532_setpin_reset(0);
            vTaskDelay(100 / portTICK_RATE_MS);
            pn532_setpin_reset(1);
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        // Start Emulator
        if (0 != nfc_emulate_target(pnd, &nfc_app_emulator, 0)) {  // contains already nfc_target_init() call
            ESP_LOGI(TAG, "transmission success");
        }
        // Close NFC device
        nfc_close(pnd);
        // Reload Delay
        vTaskDelay(100);
    }

    nfc_exit(context);
err:
    ESP_LOGE(TAG, "unrecoverable error");
    esp_restart();
}

void nfc_app_set_mode(uint8_t idx)
{
    ESP_LOGI(TAG, "mode %u", idx);

    if (idx != 0) {
        if (!nfc_app_task_handle) {
            nfc_app_init();
        }
    } else {
        if (nfc_app_task_handle) {
            nfc_app_deinit();
        }
    }
}

void nfc_app_init(void)
{
    xEventGroupSetBits(user_event_group, NFC_APP_RUN_BIT);

    xTaskCreatePinnedToCore(nfc_app_task, "NfcAppT", 5120, NULL, 9, &nfc_app_task_handle, 1);
}

void nfc_app_deinit(void)
{
    xEventGroupClearBits(user_event_group, NFC_APP_RUN_BIT);

    if (context) {
        nfc_exit(context);
        context = NULL;
    }

    pn532_setpin_reset(0);

    vTaskDelete(nfc_app_task_handle);
    nfc_app_task_handle = NULL;

    ESP_LOGI(TAG, "stopped.");
}
