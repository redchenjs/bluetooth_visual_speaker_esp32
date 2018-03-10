/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

/*
 * Peripheral : I2C1
 * SDA        : PB9
 * SCL        : PB6
 */

#ifndef _GINPUT_LLD_MOUSE_BOARD_H
#define _GINPUT_LLD_MOUSE_BOARD_H

// Avoid naming collisions with CubeHAL
#undef Red
#undef Green
#undef Blue

// Include CubeHAL
#include "stm32f4xx_hal.h"

// Resolution and Accuracy Settings
#define GMOUSE_EXC7200_PEN_CALIBRATE_ERROR		8
#define GMOUSE_EXC7200_PEN_CLICK_ERROR			6
#define GMOUSE_EXC7200_PEN_MOVE_ERROR			4
#define GMOUSE_EXC7200_FINGER_CALIBRATE_ERROR	14
#define GMOUSE_EXC7200_FINGER_CLICK_ERROR		18
#define GMOUSE_EXC7200_FINGER_MOVE_ERROR		14

// Number of bytes for the private area of this board file
#define GMOUSE_EXC7200_BOARD_DATA_SIZE			0

// The slave address of the I2C EXC7200
#define EXC7200_SLAVE_ADDR						0x08

static I2C_HandleTypeDef _i2cHandle;

static bool_t init_board(GMouse* m, unsigned driverinstance)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	(void)m;
	(void)driverinstance;

	// Enable peripheral clocks
	__HAL_RCC_I2C1_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	// SCL pin setup
	GPIO_InitStruct.Pin       = GPIO_PIN_6;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull      = GPIO_PULLUP;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	// SDA pin setup
	GPIO_InitStruct.Pin       = GPIO_PIN_9;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull      = GPIO_PULLUP;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	// Initialize the I2C peripheral
	_i2cHandle.Instance 			= I2C1;
	_i2cHandle.Init.ClockSpeed      = 400000;
	_i2cHandle.Init.DutyCycle       = I2C_DUTYCYCLE_2;
	_i2cHandle.Init.OwnAddress1     = 0;
	_i2cHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
	_i2cHandle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
	_i2cHandle.Init.OwnAddress2     = 0;
	_i2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
	_i2cHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLED;  
	HAL_I2C_Init(&_i2cHandle);
	
	return TRUE;
}

static bool_t read_bytes(GMouse* m, uint8_t reg, uint8_t* buffer, uint8_t nbrBytes)
{
	(void)m;

	HAL_I2C_Master_Transmit(&_i2cHandle, (uint16_t)EXC7200_SLAVE_ADDR, (uint8_t*)&reg, 1, 10000);
	HAL_I2C_Master_Receive(&_i2cHandle, (uint16_t)EXC7200_SLAVE_ADDR, buffer, nbrBytes, 10000);
	
	return TRUE;
}

#endif /* _GINPUT_LLD_MOUSE_BOARD_H */
