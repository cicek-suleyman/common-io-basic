//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/9/26.
//

#ifndef COMMON_IO_BASIC_BOARD_H
#define COMMON_IO_BASIC_BOARD_H

#include "hal.h"
#include "iot_gpio.h"
#include "iot_i2c.h"
#include "iot_spi.h"

extern IotGpioHandle_t ld3, ld4, ld5, ld6;
extern IotGpioHandle_t b1;
extern IotI2CHandle_t i2c1;
extern IotSPIHandle_t spi1;

int BoardInit(void);

#endif //COMMON_IO_BASIC_BOARD_H