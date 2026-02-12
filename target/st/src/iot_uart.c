//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 1/27/26.
//


#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "iot_uart.h"

#include "linked_list.h"
#include "board.h"

linked_list_t *uart_list = NULL;

typedef struct IotUARTDescriptor {
        UART_HandleTypeDef handle;
        IRQn_Type irq;
        IotUARTConfig_t config;
        IotUARTCallback_t callback;
        void * pvUserContext;
} UARTHandle_t;

void prv_iot_uart_irq() {
        const uint32_t irq = __get_IPSR() - NVIC_USER_IRQ_OFFSET;

        const linked_list_t *iter = uart_list;

        while (iter != NULL) {
                UARTHandle_t *ptr = (UARTHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (ptr->irq == irq) HAL_UART_IRQHandler(&ptr->handle);
                        return;
                }
                iter = iter->next;
        }
}

void prv_iot_uart_callback(UART_HandleTypeDef *handle, IotUARTOperationStatus_t xStatus) {
        const linked_list_t *iter = uart_list;

        while (iter != NULL) {
                const UARTHandle_t *ptr = (UARTHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (&ptr->handle == handle && ptr->callback != NULL) ptr->callback(xStatus, ptr->pvUserContext);
                        return;
                }
                iter = iter->next;
        }
}

void prv_iot_uart_error_callback(UART_HandleTypeDef *handle) {
        if (HAL_UART_GetState(handle) == HAL_UART_STATE_BUSY_RX)
                prv_iot_uart_callback(handle, eUartLastReadFailed);

        if (HAL_UART_GetState(handle) == HAL_UART_STATE_BUSY_TX)
                prv_iot_uart_callback(handle, eUartLastWriteFailed);
}

void prv_iot_uart_tx_completed_callback(UART_HandleTypeDef *handle) {
        prv_iot_uart_callback(handle, eUartWriteCompleted);
}

void prv_iot_uart_rx_completed_callback(UART_HandleTypeDef *handle) {
        prv_iot_uart_callback(handle, eUartReadCompleted);
}

IotUARTHandle_t iot_uart_open( int32_t lUartInstance ) {
        UARTHandle_t *pxHandle = malloc(sizeof(UARTHandle_t));
        memset(pxHandle, 0, sizeof(UARTHandle_t));

        USART_TypeDef *instance = NULL;

        if (pxHandle == NULL) {
                return NULL;
        }

        switch (lUartInstance) {
                case 1: instance = USART1; __HAL_RCC_USART1_CLK_ENABLE(); pxHandle->irq = USART1_IRQn; break;
                case 2: instance = USART2; __HAL_RCC_USART2_CLK_ENABLE(); pxHandle->irq = USART2_IRQn; break;
#ifdef USART3
                case 3: instance = USART3; __HAL_RCC_USART3_CLK_ENABLE(); pxHandle->irq = USART3_IRQn; break;
#endif
#ifdef UART4
                case 4: instance = UART4; __HAL_RCC_UART4_CLK_ENABLE(); pxHandle->irq = UART4_IRQn; break;
#endif
#ifdef UART5
                case 5: instance = UART5; __HAL_RCC_UART5_CLK_ENABLE(); pxHandle->irq = UART5_IRQn; break;
#endif
#ifdef USART6
                case 6: instance = USART6; __HAL_RCC_USART6_CLK_ENABLE(); pxHandle->irq = USART6_IRQn; break;
#endif
                default: free( pxHandle ); return NULL;
        }

        const linked_list_t *iter = uart_list;

        while (iter != NULL) {
                const UARTHandle_t *ptr = (UARTHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (ptr->handle.Instance == instance) // already registered
                                return (IotUARTHandle_t)ptr;
                }
                iter = iter->next;
        }
        extern volatile uint32_t g_pfnRamVectors[];
        g_pfnRamVectors[NVIC_USER_IRQ_OFFSET + pxHandle->irq] = (uint32_t)((uint32_t *)prv_iot_uart_irq);

        pxHandle->handle.Instance = instance;
        pxHandle->handle.Init.Mode = UART_MODE_TX_RX;
        pxHandle->handle.Init.OverSampling = UART_OVERSAMPLING_16;

        pxHandle->handle.ErrorCallback = prv_iot_uart_error_callback;
        pxHandle->handle.TxCpltCallback = prv_iot_uart_tx_completed_callback;
        pxHandle->handle.RxCpltCallback = prv_iot_uart_rx_completed_callback;

        HAL_NVIC_SetPriority(pxHandle->irq, 0, 0);
        HAL_NVIC_EnableIRQ(pxHandle->irq);

        uart_list = ll_append(uart_list, pxHandle);

        return (IotUARTHandle_t)pxHandle;
}

void iot_uart_set_callback( IotUARTHandle_t const pxUartPeripheral,
                            IotUARTCallback_t xCallback,
                            void * pvUserContext ) {
        if (pxUartPeripheral == NULL) return;

        UARTHandle_t *pxHandle = (UARTHandle_t *)pxUartPeripheral;
        pxHandle->callback = xCallback;
        pxHandle->pvUserContext = pvUserContext;
}

int32_t iot_uart_read_sync( IotUARTHandle_t const pxUartPeripheral,
                            uint8_t * const pvBuffer,
                            size_t xBytes ) {
        if (pxUartPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_UART_INVALID_VALUE;

        UARTHandle_t *pxHandle = (UARTHandle_t *)pxUartPeripheral;

        if (HAL_UART_GetState(&pxHandle->handle) == HAL_UART_STATE_RESET)
                return IOT_UART_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_UART_Receive(&pxHandle->handle, pvBuffer, xBytes, 1000);
        if (status != HAL_OK) {
                if (status == HAL_BUSY) return IOT_UART_BUSY;

                return IOT_UART_READ_FAILED;
        }

        return IOT_UART_SUCCESS;
}

int32_t iot_uart_write_sync( IotUARTHandle_t const pxUartPeripheral,
                             uint8_t * const pvBuffer,
                             size_t xBytes ) {
        if (pxUartPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_UART_INVALID_VALUE;

        UARTHandle_t *pxHandle = (UARTHandle_t *)pxUartPeripheral;

        if (HAL_UART_GetState(&pxHandle->handle) == HAL_UART_STATE_RESET)
                return IOT_UART_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_UART_Transmit(&pxHandle->handle, pvBuffer, xBytes, 1000);

        if (status != HAL_OK)
                return IOT_UART_WRITE_FAILED;

        return IOT_UART_SUCCESS;
}

int32_t iot_uart_read_async( IotUARTHandle_t const pxUartPeripheral,
                             uint8_t * const pvBuffer,
                             size_t xBytes ) {
        if (pxUartPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_UART_INVALID_VALUE;

        UARTHandle_t *pxHandle = (UARTHandle_t *)pxUartPeripheral;

        if (HAL_UART_GetState(&pxHandle->handle) == HAL_UART_STATE_RESET)
                return IOT_UART_INVALID_VALUE;

        HAL_StatusTypeDef status;
        if (pxHandle->handle.hdmatx != NULL && pxHandle->handle.hdmatx->State == HAL_DMA_STATE_READY)
                status = HAL_UART_Receive_DMA(&pxHandle->handle, pvBuffer, xBytes);
        else
                status = HAL_UART_Receive_IT(&pxHandle->handle, pvBuffer, xBytes);

        if (status != HAL_OK) {
                if (status == HAL_BUSY) return IOT_UART_BUSY;

                return IOT_UART_READ_FAILED;
        }

        return IOT_UART_SUCCESS;
}

int32_t iot_uart_write_async( IotUARTHandle_t const pxUartPeripheral,
                              uint8_t * const pvBuffer,
                              size_t xBytes ) {
        if (pxUartPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_UART_INVALID_VALUE;

        UARTHandle_t *pxHandle = (UARTHandle_t *)pxUartPeripheral;

        if (HAL_UART_GetState(&pxHandle->handle) == HAL_UART_STATE_RESET)
                return IOT_UART_INVALID_VALUE;

        HAL_StatusTypeDef status;
        if (pxHandle->handle.hdmatx != NULL && pxHandle->handle.hdmatx->State == HAL_DMA_STATE_READY)
                status = HAL_UART_Transmit_DMA(&pxHandle->handle, pvBuffer, xBytes);
        else
                status = HAL_UART_Transmit_IT(&pxHandle->handle, pvBuffer, xBytes);

        if (status != HAL_OK) {
                if (status == HAL_BUSY) return IOT_UART_BUSY;

                return IOT_UART_WRITE_FAILED;
        }

        return IOT_UART_SUCCESS;
}

int32_t iot_uart_ioctl( IotUARTHandle_t const pxUartPeripheral,
                        IotUARTIoctlRequest_t xUartRequest,
                        void * const pvBuffer ) {
        if (pxUartPeripheral == NULL || pvBuffer == NULL)
                return IOT_UART_INVALID_VALUE;

        UARTHandle_t *pxHandle = (UARTHandle_t *)pxUartPeripheral;

        if (pxHandle->handle.Instance == NULL)
                return IOT_UART_INVALID_VALUE;

        switch (xUartRequest) {
                case eUartSetConfig: {
                        IotUARTConfig_t *config = (IotUARTConfig_t *)pvBuffer;
                        pxHandle->config = *config;
                        pxHandle->handle.Init.BaudRate = config->ulBaudrate;

                        if (config->xParity == eUartParityNone) pxHandle->handle.Init.Parity = UART_PARITY_NONE;
                        else if (config->xParity == eUartParityEven) pxHandle->handle.Init.Parity = UART_PARITY_EVEN;
                        else if (config->xParity == eUartParityOdd) pxHandle->handle.Init.Parity = UART_PARITY_ODD;
                        else return IOT_UART_INVALID_VALUE;

                        if (config->xStopbits == eUartStopBitsOne) pxHandle->handle.Init.StopBits = UART_STOPBITS_1;
                        else if (config->xStopbits == eUartStopBitsTwo) pxHandle->handle.Init.StopBits = UART_STOPBITS_2;
                        else return IOT_UART_INVALID_VALUE;

                        if (config->ucWordlength == 8) pxHandle->handle.Init.WordLength = UART_WORDLENGTH_8B;
                        else if (config->ucWordlength == 9) pxHandle->handle.Init.WordLength = UART_WORDLENGTH_9B;
                        else return IOT_UART_INVALID_VALUE;

                        if (config->ucFlowControl == 0) pxHandle->handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
                        else if (config->ucFlowControl == 1) pxHandle->handle.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
                        else if (config->ucFlowControl == 2) pxHandle->handle.Init.HwFlowCtl = UART_HWCONTROL_RTS;
                        else if (config->ucFlowControl == 3) pxHandle->handle.Init.HwFlowCtl = UART_HWCONTROL_CTS;
                        else return IOT_UART_INVALID_VALUE;

                        if (HAL_UART_Init(&pxHandle->handle) != HAL_OK) return IOT_UART_INVALID_VALUE;
                } break;
                case eUartGetConfig: {
                        IotUARTConfig_t *config = (IotUARTConfig_t *)pvBuffer;
                        *config = pxHandle->config;
                        /*config->ulBaudrate = pxHandle->handle.Init.BaudRate;

                        if (pxHandle->handle.Init.Parity == UART_PARITY_NONE) config->xParity = eUartParityNone;
                        else if (pxHandle->handle.Init.Parity == UART_PARITY_EVEN) config->xParity = eUartParityEven;
                        else if (pxHandle->handle.Init.Parity == UART_PARITY_ODD) config->xParity = eUartParityOdd;
                        else return IOT_UART_INVALID_VALUE;

                        if (pxHandle->handle.Init.StopBits == UART_STOPBITS_1) config->xStopbits = eUartStopBitsOne;
                        else if (pxHandle->handle.Init.StopBits == UART_STOPBITS_2) config->xStopbits = eUartStopBitsTwo;
                        else return IOT_UART_INVALID_VALUE;

                        if (pxHandle->handle.Init.WordLength == UART_WORDLENGTH_8B) config->ucWordlength = 8;
                        else if (pxHandle->handle.Init.WordLength == UART_WORDLENGTH_9B) config->ucWordlength = 9;
                        else return IOT_UART_INVALID_VALUE;

                        if (pxHandle->handle.Init.HwFlowCtl == UART_HWCONTROL_NONE) config->ucFlowControl = 0;
                        else if (pxHandle->handle.Init.HwFlowCtl == UART_HWCONTROL_RTS_CTS) config->ucFlowControl = 1;
                        else if (pxHandle->handle.Init.HwFlowCtl == UART_HWCONTROL_RTS) config->ucFlowControl = 2;
                        else if (pxHandle->handle.Init.HwFlowCtl == UART_HWCONTROL_CTS) config->ucFlowControl = 3;
                        else return IOT_UART_INVALID_VALUE;*/
                } break;
                case eGetTxNoOfbytes: *(uint16_t *)pvBuffer = pxHandle->handle.TxXferSize; break;
                case eGetRxNoOfbytes: *(uint16_t *)pvBuffer = pxHandle->handle.RxXferSize; break;
                default: return IOT_UART_INVALID_VALUE;
        }

        return IOT_UART_SUCCESS;
}

int32_t iot_uart_cancel( IotUARTHandle_t const pxUartPeripheral ) {
        if (pxUartPeripheral == NULL)
                return IOT_UART_INVALID_VALUE;

        UARTHandle_t *pxHandle = (UARTHandle_t *)pxUartPeripheral;

        if (HAL_UART_GetState(&pxHandle->handle) == HAL_UART_STATE_RESET)
                return IOT_UART_INVALID_VALUE;

        if (HAL_UART_GetState(&pxHandle->handle) == HAL_UART_STATE_READY)
                return IOT_UART_NOTHING_TO_CANCEL;

        if (pxHandle->handle.Instance->CR1 & USART_CR1_TXEIE || pxHandle->handle.Instance->CR1 & USART_CR1_RXNEIE) {
                HAL_UART_Abort_IT(&pxHandle->handle);
        }
        else HAL_UART_Abort(&pxHandle->handle);

        return IOT_UART_SUCCESS;
}

int32_t iot_uart_close( IotUARTHandle_t const pxUartPeripheral ) {
        if (pxUartPeripheral == NULL)
                return IOT_UART_INVALID_VALUE;

        UARTHandle_t *pxHandle = (UARTHandle_t *)pxUartPeripheral;

        if (HAL_UART_GetState(&pxHandle->handle) == HAL_UART_STATE_RESET)
                return IOT_UART_INVALID_VALUE;

        HAL_UART_DeInit(&pxHandle->handle);

        extern volatile uint32_t g_pfnRamVectors[];
        g_pfnRamVectors[NVIC_USER_IRQ_OFFSET + pxHandle->irq] = 0x00;
        uart_list = ll_remove(uart_list, pxHandle);
        free( pxHandle );

        return IOT_UART_SUCCESS;
}
