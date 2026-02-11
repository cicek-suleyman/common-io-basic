//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/11/26.
//

#ifndef COMMON_IO_BASIC_BOARD_H
#define COMMON_IO_BASIC_BOARD_H

#include "hal.h"
#include "iot_uart.h"
#include "iot_gpio.h"

extern IotGpioHandle_t b1;
extern IotGpioHandle_t ld2;
extern IotUARTHandle_t uart2;

int BoardInit(void);

#endif //COMMON_IO_BASIC_BOARD_H