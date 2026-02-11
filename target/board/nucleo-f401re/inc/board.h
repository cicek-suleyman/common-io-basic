//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/9/26.
//

#ifndef COMMON_IO_BASIC_BOARD_H
#define COMMON_IO_BASIC_BOARD_H

#include "hal.h"
#include "iot_gpio.h"
#include "iot_i2c.h"
#include "iot_uart.h"

extern IotGpioHandle_t ld2;
extern IotGpioHandle_t b1;
extern IotUARTHandle_t uart2;
extern IotI2CHandle_t i2c1;

int BoardInit(void);

#endif //COMMON_IO_BASIC_BOARD_H