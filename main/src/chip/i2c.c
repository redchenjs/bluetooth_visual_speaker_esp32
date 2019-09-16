/*
 * i2c.c
 *
 *  Created on: 2019-04-18 20:27
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "driver/i2c.h"

#define I2C0_TAG "i2c-0"

#ifdef CONFIG_ENABLE_NFC_BT_PAIRING
void i2c0_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_PN532_TX_SDA_PIN,
        .scl_io_num = CONFIG_PN532_RX_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));
    ESP_ERROR_CHECK(i2c_set_timeout(I2C_NUM_0, 80 * (I2C_APB_CLK_FREQ / conf.master.clk_speed)));

    ESP_LOGI(I2C0_TAG, "initialized, sda: %d, scl: %d",
             conf.sda_io_num,
             conf.scl_io_num
    );
}
#endif
