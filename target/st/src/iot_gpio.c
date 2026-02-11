//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 1/24/26.
//

#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

#include "iot_gpio.h"

#include "hal.h"
#include "linked_list.h"


typedef struct {
        IotGpioCallback_t callback;
        void * pvUserContext;
        IRQn_Type irq;
} GpioExtiHandle_t;

typedef struct IotGpioDescriptor {
        GPIO_TypeDef  *instance;
        GPIO_InitTypeDef init;
        GpioExtiHandle_t *exti;
} GpioHandle_t;

linked_list_t *gpio_exti_list = NULL;

void prv_iot_gpio_irq() {
        const uint32_t irq = __get_IPSR() - NVIC_USER_IRQ_OFFSET;

        const linked_list_t *iter = gpio_exti_list;

        while (iter != NULL) {
                GpioHandle_t *ptr = (GpioHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (ptr->exti->irq == irq) {
                                HAL_GPIO_EXTI_IRQHandler(ptr->init.Pin);
                                int32_t state = HAL_GPIO_ReadPin(ptr->instance, ptr->init.Pin);
                                if (ptr->exti->callback != NULL)
                                        ptr->exti->callback(state, ptr->exti->pvUserContext);
                        }
                }
                iter = iter->next;
        }
}

IotGpioHandle_t iot_gpio_open( int32_t lGpioNumber ) {
        GpioHandle_t *pxHandle = malloc( sizeof( GpioHandle_t ) );
        memset(pxHandle, 0, sizeof(GpioHandle_t));

        if( pxHandle == NULL )
        {
                return NULL;
        }

        GPIO_TypeDef *port = NULL;
        const uint32_t pin = 0x01 << (lGpioNumber & 0x0F);

        switch( (uint8_t)((lGpioNumber >> 4) & 0x0F) ) {
                case 0: port = GPIOA; __HAL_RCC_GPIOA_CLK_ENABLE(); break;
                case 1: port = GPIOB; __HAL_RCC_GPIOB_CLK_ENABLE(); break;
                case 2: port = GPIOC; __HAL_RCC_GPIOC_CLK_ENABLE(); break;
                case 3: port = GPIOD; __HAL_RCC_GPIOD_CLK_ENABLE(); break;
#ifdef GPIOE
                case 4: port = GPIOE; __HAL_RCC_GPIOE_CLK_ENABLE(); break;
#endif
#ifdef GPIOF
                case 5: port = GPIOF; __HAL_RCC_GPIOF_CLK_ENABLE(); break;
#endif
#ifdef GPIOG
                case 6: port = GPIOG; __HAL_RCC_GPIOG_CLK_ENABLE(); break;
#endif
#ifdef GPIOH
                case 7: port = GPIOH; __HAL_RCC_GPIOH_CLK_ENABLE(); break;
#endif
                default: {
                        free( pxHandle );
                        return NULL;
                }
        }

        if (!IS_GPIO_ALL_INSTANCE(port) || !IS_GPIO_PIN(pin)) {
                free( pxHandle );
                return NULL;
        }

        pxHandle->instance = port;
        pxHandle->init.Pin = pin;
        pxHandle->init.Speed = GPIO_SPEED_FREQ_LOW;

        return ( IotGpioHandle_t ) pxHandle;
}

void iot_gpio_set_callback( IotGpioHandle_t const pxGpio,
                            IotGpioCallback_t xGpioCallback,
                            void * pvUserContext ) {
        if (pxGpio == NULL)
                return;

        GpioHandle_t *pxHandle = (GpioHandle_t *)pxGpio;
        const int32_t pin = __builtin_ctz(pxHandle->init.Pin);
        uint32_t irq = 0;
        switch (pin) {
                case 0: irq = EXTI0_IRQn; break;
                case 1: irq = EXTI1_IRQn; break;
                case 2: irq = EXTI2_IRQn; break;
                case 3: irq = EXTI3_IRQn; break;
                case 4: irq = EXTI4_IRQn; break;
                case 5: case 6: case 7: case 8:
                case 9: irq = EXTI9_5_IRQn; break;
                case 10: case 11: case 12: case 13: case 14:
                case 15: irq = EXTI15_10_IRQn; break;
                default: return;
        }

        if (pxHandle->exti == NULL)
                pxHandle->exti = malloc(sizeof(GpioExtiHandle_t));

        pxHandle->exti->irq = irq;
        pxHandle->exti->callback = xGpioCallback;
        pxHandle->exti->pvUserContext = pvUserContext;

        const linked_list_t *ins = ll_search(gpio_exti_list, pxHandle, NULL);
        if (ins == NULL) {
                gpio_exti_list = ll_append(gpio_exti_list, pxHandle);
                extern volatile uint32_t g_pfnRamVectors[];
                g_pfnRamVectors[pxHandle->exti->irq + NVIC_USER_IRQ_OFFSET] = (uint32_t)((uint32_t *)prv_iot_gpio_irq);
        }

        HAL_NVIC_SetPriority(pxHandle->exti->irq, 0, 0);
        HAL_NVIC_EnableIRQ(pxHandle->exti->irq);
}

int32_t iot_gpio_read_sync( IotGpioHandle_t const pxGpio,
                            uint8_t * pucPinState ) {
        if (pxGpio == NULL || pucPinState == NULL)
                return IOT_GPIO_INVALID_VALUE;

        const GpioHandle_t *pxHandle = (GpioHandle_t *)pxGpio;
        *pucPinState = HAL_GPIO_ReadPin(pxHandle->instance, pxHandle->init.Pin);

        return IOT_GPIO_SUCCESS;
}

int32_t iot_gpio_write_sync( IotGpioHandle_t const pxGpio,
                             uint8_t ucPinState ) {
        if (pxGpio == NULL)
                return IOT_GPIO_INVALID_VALUE;

        const GpioHandle_t *pxHandle = (GpioHandle_t *)pxGpio;
        HAL_GPIO_WritePin(pxHandle->instance, pxHandle->init.Pin, (ucPinState == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET));
        return  0;
}

int32_t iot_gpio_ioctl( IotGpioHandle_t const pxGpio,
                        IotGpioIoctlRequest_t xRequest,
                        void * const pvBuffer ) {
        if (pxGpio == NULL || pvBuffer == NULL)
                return IOT_GPIO_INVALID_VALUE;

        GpioHandle_t *pxHandle = (GpioHandle_t *)pxGpio;

        switch (xRequest) {
                case eSetGpioFunction: {
#if defined(STM32F1)
                        return IOT_GPIO_FUNCTION_NOT_SUPPORTED;
#else
                        const uint32_t func = *(uint32_t *)pvBuffer;
                        if (!IS_GPIO_AF(func))
                                return IOT_GPIO_INVALID_VALUE;

                        pxHandle->init.Mode = GPIO_MODE_AF_PP;

                        pxHandle->init.Alternate = func;

                        HAL_GPIO_Init(pxHandle->instance, &(pxHandle->init));
#endif
                } break;
                case eSetGpioDirection: {
                        const IotGpioDirection_t dir = *(IotGpioDirection_t *)pvBuffer;
                        if (dir == eGpioDirectionInput) pxHandle->init.Mode = GPIO_MODE_INPUT;
                        else if (dir == eGpioDirectionOutput) pxHandle->init.Mode = GPIO_MODE_OUTPUT_PP;
                        else return IOT_GPIO_INVALID_VALUE;

                        HAL_GPIO_Init(pxHandle->instance, &(pxHandle->init));
                } break;
                case eSetGpioPull: {
                        const IotGpioPull_t pull = *(IotGpioPull_t *)pvBuffer;
                        if (pull == eGpioPullNone) pxHandle->init.Pull = GPIO_NOPULL;
                        else if (pull == eGpioPullUp) pxHandle->init.Pull = GPIO_PULLUP;
                        else if (pull == eGpioPullDown) pxHandle->init.Pull = GPIO_PULLDOWN;
                        else return IOT_GPIO_INVALID_VALUE;

                        HAL_GPIO_Init(pxHandle->instance, &(pxHandle->init));
                } break;
                case eSetGpioOutputMode: {
                        const IotGpioOutputMode_t mode = *(IotGpioOutputMode_t *)pvBuffer;

                        if ((pxHandle->init.Mode & GPIO_MODE_OUTPUT_PP) == GPIO_MODE_OUTPUT_PP) {
                                if (mode == eGpioPushPull) pxHandle->init.Mode = GPIO_MODE_OUTPUT_PP;
                                else if (mode == eGpioOpenDrain) pxHandle->init.Mode = GPIO_MODE_OUTPUT_OD;
                                else return IOT_GPIO_INVALID_VALUE;
                        } else if ((pxHandle->init.Mode & GPIO_MODE_AF_PP) == GPIO_MODE_AF_PP) {
                                if (mode == eGpioPushPull) pxHandle->init.Mode = GPIO_MODE_AF_PP;
                                else if (mode == eGpioOpenDrain) pxHandle->init.Mode = GPIO_MODE_AF_OD;
                                else return IOT_GPIO_INVALID_VALUE;
                        } else return IOT_GPIO_INVALID_VALUE;

                        HAL_GPIO_Init(pxHandle->instance, &(pxHandle->init));
                } break;
                case eSetGpioInterrupt: {
                        const IotGpioInterrupt_t it = *(IotGpioInterrupt_t *)pvBuffer;

                        if ((pxHandle->init.Mode & GPIO_MODE_OUTPUT_PP) != GPIO_MODE_OUTPUT_PP) {
                                switch (it) {
                                        case eGpioInterruptRising: pxHandle->init.Mode = GPIO_MODE_IT_RISING; break;
                                        case eGpioInterruptFalling: pxHandle->init.Mode = GPIO_MODE_IT_FALLING; break;
                                        case eGpioInterruptEdge: pxHandle->init.Mode = GPIO_MODE_IT_RISING_FALLING; break;
                                        // case eGpioInterruptLow: break;
                                        // case eGpioInterruptHigh: break;
                                        default: return IOT_GPIO_FUNCTION_NOT_SUPPORTED;
                                }
                        } else return IOT_GPIO_INVALID_VALUE;

                        HAL_GPIO_Init(pxHandle->instance, &(pxHandle->init));
                } break;
                case eSetGpioSpeed: {
                        const uint32_t speed = *(uint32_t *)pvBuffer;
                        if (speed > 0x03)
                                return IOT_GPIO_INVALID_VALUE;

                        pxHandle->init.Speed = speed;

                        HAL_GPIO_Init(pxHandle->instance, &(pxHandle->init));
                } break;
                case eSetGpioDriveStrength: return IOT_GPIO_FUNCTION_NOT_SUPPORTED;
                case eGetGpioFunction: {
#if defined(STM32F1)
                        return IOT_GPIO_FUNCTION_NOT_SUPPORTED;
#else
                        *(uint32_t *)pvBuffer = pxHandle->init.Alternate;
#endif
                } break;
                case eGetGpioDirection: {
                        if ((pxHandle->init.Mode & GPIO_MODE_OUTPUT_PP) == GPIO_MODE_OUTPUT_PP) *(IotGpioDirection_t *)pvBuffer = eGpioDirectionOutput;
                        else *(IotGpioDirection_t *)pvBuffer = eGpioDirectionInput;
                } break;
                case eGetGpioPull: {
                        if ((pxHandle->init.Pull & GPIO_NOPULL) == GPIO_NOPULL) *(IotGpioPull_t *)pvBuffer = eGpioPullNone;
                        else if ((pxHandle->init.Pull & GPIO_PULLUP) == GPIO_PULLUP) *(IotGpioPull_t *)pvBuffer = eGpioPullUp;
                        else if ((pxHandle->init.Pull & GPIO_PULLDOWN) == GPIO_PULLDOWN) *(IotGpioPull_t *)pvBuffer = eGpioPullDown;
                        else return IOT_GPIO_INVALID_VALUE;
                } break;
                case eGetGpioOutputType: {
                        if ((pxHandle->init.Mode & GPIO_MODE_OUTPUT_PP) != GPIO_MODE_OUTPUT_PP)
                                return IOT_GPIO_INVALID_VALUE;

                        if ((pxHandle->init.Mode & GPIO_MODE_OUTPUT_OD) == GPIO_MODE_OUTPUT_OD) *(IotGpioOutputMode_t *)pvBuffer = eGpioOpenDrain;
                        else if ((pxHandle->init.Mode & GPIO_MODE_OUTPUT_PP) == GPIO_MODE_OUTPUT_PP) *(IotGpioOutputMode_t *)pvBuffer = eGpioPushPull;
                } break;
                case eGetGpioInterrupt: {
                        if (pxHandle->exti == NULL || !(pxHandle->init.Mode & GPIO_MODE_IT_RISING))
                                *(IotGpioInterrupt_t *)pvBuffer = eGpioInterruptNone;
                        else {
                                if ((pxHandle->init.Mode & GPIO_MODE_IT_RISING_FALLING) == GPIO_MODE_IT_RISING_FALLING)
                                        *(IotGpioInterrupt_t *)pvBuffer = eGpioInterruptEdge;
                                else if ((pxHandle->init.Mode & GPIO_MODE_IT_RISING) == GPIO_MODE_IT_RISING)
                                        *(IotGpioInterrupt_t *)pvBuffer = eGpioInterruptRising;
                                else if ((pxHandle->init.Mode & GPIO_MODE_IT_FALLING) == GPIO_MODE_IT_FALLING)
                                        *(IotGpioInterrupt_t *)pvBuffer = eGpioInterruptFalling;
                                else return IOT_GPIO_FUNCTION_NOT_SUPPORTED;
                        }
                } break;
                case eGetGpioSpeed: {
                        *(uint32_t *)pvBuffer = pxHandle->init.Speed;
                } break;
                case eGetGpioDriveStrength: return IOT_GPIO_FUNCTION_NOT_SUPPORTED; break;
                default: return IOT_GPIO_INVALID_VALUE;
        }


        return IOT_GPIO_SUCCESS;
}

int32_t iot_gpio_close( IotGpioHandle_t const pxGpio ) {
        if (pxGpio == NULL)
                return IOT_GPIO_INVALID_VALUE;

        GpioHandle_t *pxHandle = (GpioHandle_t *)pxGpio;
        HAL_GPIO_DeInit(pxHandle->instance, pxHandle->init.Pin);

        if (pxHandle->exti != NULL) {
                // extern volatile uint32_t g_pfnRamVectors[];
                // g_pfnRamVectors[pxHandle->exti->irq + NVIC_USER_IRQ_OFFSET] = 0x00; // 5-9 ve 10-15 ortak olduğu için NULL yapılmamalı
                free( pxHandle->exti );
                gpio_exti_list = ll_remove(gpio_exti_list, pxHandle);
        }
        if (pxHandle->exti != NULL)
                free(pxHandle->exti);
        free( pxHandle );

        return IOT_GPIO_SUCCESS;
}
