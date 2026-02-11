//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/7/26.
//

#ifndef COMMON_IO_BASIC_HAL_H
#define COMMON_IO_BASIC_HAL_H

#if defined(STM32F4)
#include "stm32f4xx_hal.h"
#elif defined(STM32F1)
#include "stm32f1xx_hal.h"
#endif

#endif //COMMON_IO_BASIC_HAL_H