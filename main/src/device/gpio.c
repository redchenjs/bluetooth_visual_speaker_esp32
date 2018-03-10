/*
 * gpio.c
 *
 *  Created on: 2018-02-10 16:10
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "driver/gpio.h"

#define LED1_PIN 25

#define GPIO_OUTPUT_PIN_SEL (1ULL<<LED1_PIN)

void gpio0_init(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_PIN_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = GPIO_OUTPUT_PIN_SEL,
        .pull_down_en = 0,
        .pull_up_en = 0
    };

    gpio_config(&io_conf);
}
