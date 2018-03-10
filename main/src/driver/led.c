/*
 * led.c
 *
 *  Created on: 2018-02-13 15:53
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "driver/gpio.h"

#define LED_INDICATOR_PIN 25

void led_indicator_on(void)
{
    gpio_set_level(LED_INDICATOR_PIN, 1);
}

void led_indicator_off(void)
{
    gpio_set_level(LED_INDICATOR_PIN, 0);
}

void led_init(void)
{
    led_indicator_on();
}