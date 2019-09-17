/*
 * uart.c
 *
 *  Created on: 2018-02-10 16:09
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

#define UART1_TAG "uart-1"

#ifdef CONFIG_ENABLE_NFC_BT_PAIRING
void uart1_init(void)
{
    uart_config_t uart_config = {
       .baud_rate = 115200,
       .data_bits = UART_DATA_8_BITS,
       .parity = UART_PARITY_DISABLE,
       .stop_bits = UART_STOP_BITS_1,
       .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1,
                                 CONFIG_PN532_RX_PIN,
                                 CONFIG_PN532_TX_PIN,
                                 UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE)
    );

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 128 * 2, 128 * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_flush_input(UART_NUM_1));

    ESP_LOGI(UART1_TAG, "initialized, tx: %d, rx: %d", CONFIG_PN532_RX_PIN, CONFIG_PN532_TX_PIN);
}
#endif
