//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 1/27/26.
//

#include "iot_power.h"

#include <stdlib.h>
#include <memory.h>
#include "board.h"

IotPowerHandle_t *power = NULL;

IotPowerHandle_t iot_power_open( void ) {
        if (power == NULL) {
                BoardInit();
                power = (IotPowerHandle_t *)malloc(sizeof(IotPowerHandle_t));
        }
        return (IotPowerHandle_t)power;
}
