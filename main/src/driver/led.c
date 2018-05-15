/*
 * led.c
 *
 *  Created on: 2018-02-13 15:53
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "driver/gpio.h"

#define LED0_PIN CONFIG_LED_PIN

void led_on(void)
{
    gpio_set_level(LED0_PIN, 1);
}

void led_off(void)
{
    gpio_set_level(LED0_PIN, 0);
}

void led_init(void)
{
    gpio_set_direction(LED0_PIN, GPIO_MODE_OUTPUT);
    led_on();
}
