//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/13/26.
//

#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "linked_list.h"

#include "iot_i2s.h"

#ifdef HAL_I2S_MODULE_ENABLED

typedef struct IotI2SDescriptor {
        I2S_HandleTypeDef handle;
        IRQn_Type irq;
        IotI2sIoctlConfig_t config;
        IotI2SCallback_t callback;
        void * pvUserContext;
} I2SHandle_t;

linked_list_t *i2s_list;

void prv_iot_i2s_irq() {
        const uint32_t irq = __get_IPSR() - NVIC_USER_IRQ_OFFSET;
        const linked_list_t *iter = i2s_list;

        while (iter != NULL) {
                I2SHandle_t *ptr = (I2SHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (ptr->irq == irq) HAL_I2S_IRQHandler(&ptr->handle);
                        return;
                }
                iter = iter->next;
        }
}

void prv_iot_i2s_callback(I2S_HandleTypeDef *hi2s, IotI2SOperationStatus_t xOpStatus) {
        const linked_list_t *iter = i2s_list;

        while (iter != NULL) {
                const I2SHandle_t *ptr = (I2SHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (&ptr->handle == hi2s && ptr->callback != NULL) ptr->callback(xOpStatus, ptr->pvUserContext);
                        return;
                }
                iter = iter->next;
        }
}

void prv_iot_i2s_transfer_completed_callback(I2S_HandleTypeDef *hi2s) {
        prv_iot_i2s_callback(hi2s, eI2SCompleted);
}

void prv_iot_i2s_error_callback(I2S_HandleTypeDef *hi2s) {
        if (HAL_I2S_GetState(hi2s) == HAL_I2S_STATE_BUSY_TX)
                prv_iot_i2s_callback(hi2s, eI2SLastWriteFailed);
        else if (HAL_I2S_GetState(hi2s) == HAL_I2S_STATE_BUSY_RX)
                prv_iot_i2s_callback(hi2s, eI2SLastReadFailed);
}

IotI2SHandle_t iot_i2s_open( int32_t lI2SInstance ) {
        I2SHandle_t *pxHandle = malloc(sizeof(I2SHandle_t));
        memset(pxHandle, 0, sizeof(I2SHandle_t));
        SPI_TypeDef *instance = NULL;

        if (pxHandle == NULL) {
                return NULL;
        }

        switch (lI2SInstance) {
                case 1: instance = SPI1; pxHandle->irq = SPI1_IRQn; break;
                case 2: instance = SPI2; pxHandle->irq = SPI2_IRQn; break;
#ifdef SPI3
                case 3: instance = SPI3; pxHandle->irq = SPI3_IRQn; break;
#endif
                default: free( pxHandle ); return NULL;
        }

        const linked_list_t *iter = i2s_list;

        while (iter != NULL) {
                const I2SHandle_t *ptr = (I2SHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (ptr->handle.Instance == instance) // already registered
                                return (IotI2SHandle_t)ptr;
                }
                iter = iter->next;
        }
        extern volatile uint32_t g_pfnRamVectors[];
        g_pfnRamVectors[NVIC_USER_IRQ_OFFSET + pxHandle->irq] = (uint32_t)((uint32_t *)prv_iot_i2s_irq);

        pxHandle->handle.Instance = instance;
        pxHandle->handle.Init.Mode = I2S_MODE_MASTER_TX;
        pxHandle->handle.Init.Standard = I2S_STANDARD_PHILIPS;
        pxHandle->handle.Init.DataFormat = I2S_DATAFORMAT_16B;
        pxHandle->handle.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
        pxHandle->handle.Init.AudioFreq = I2S_AUDIOFREQ_96K;
        pxHandle->handle.Init.CPOL = I2S_CPOL_LOW;
        pxHandle->handle.Init.ClockSource = I2S_CLOCK_PLL;
        pxHandle->handle.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

        pxHandle->handle.ErrorCallback = prv_iot_i2s_error_callback;
        pxHandle->handle.TxCpltCallback = prv_iot_i2s_transfer_completed_callback;
        pxHandle->handle.RxCpltCallback = prv_iot_i2s_transfer_completed_callback;
        pxHandle->handle.TxRxCpltCallback = prv_iot_i2s_transfer_completed_callback;

        HAL_I2S_Init(&pxHandle->handle);

        HAL_NVIC_SetPriority(pxHandle->irq, 0, 0);
        HAL_NVIC_EnableIRQ(pxHandle->irq);

        i2s_list = ll_append(i2s_list, pxHandle);

        return (IotI2SHandle_t)pxHandle;
}

void iot_i2s_set_callback( IotI2SHandle_t const pxI2SPeripheral,
                           IotI2SCallback_t xCallback,
                           void * pvUserContext ) {
        if (pxI2SPeripheral == NULL)
                return;

        I2SHandle_t *pxHandle = (I2SHandle_t *)pxI2SPeripheral;
        pxHandle->callback = xCallback;
        pxHandle->pvUserContext = pvUserContext;
}

int32_t iot_i2s_read_async( IotI2SHandle_t const pxI2SPeripheral,
                            uint8_t * const pvBuffer,
                            size_t xBytes ) {
        if (pxI2SPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_I2S_INVALID_VALUE;

        I2SHandle_t *pxHandle = (I2SHandle_t *)pxI2SPeripheral;

        if (HAL_I2S_GetState(&pxHandle->handle) == HAL_I2S_STATE_RESET)
                return IOT_I2S_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_I2S_Receive_IT(&pxHandle->handle, (uint16_t *)pvBuffer, xBytes/2);
        if (status != HAL_OK)
                return IOT_I2S_READ_FAILED;

        return IOT_I2S_SUCCESS;
}

int32_t iot_i2s_write_async( IotI2SHandle_t const pxI2SPeripheral,
                             uint8_t * const pvBuffer,
                             size_t xBytes ) {
        if (pxI2SPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_I2S_INVALID_VALUE;

        I2SHandle_t *pxHandle = (I2SHandle_t *)pxI2SPeripheral;

        if (HAL_I2S_GetState(&pxHandle->handle) == HAL_I2S_STATE_RESET)
                return IOT_I2S_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_I2S_Transmit_IT(&pxHandle->handle, (uint16_t *)pvBuffer, xBytes/2);
        if (status != HAL_OK)
                return IOT_I2S_READ_FAILED;

        return IOT_I2S_SUCCESS;
}

int32_t iot_i2s_read_sync( IotI2SHandle_t const pxI2SPeripheral,
                           uint8_t * const pvBuffer,
                           size_t xBytes ) {
        if (pxI2SPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_I2S_INVALID_VALUE;

        I2SHandle_t *pxHandle = (I2SHandle_t *)pxI2SPeripheral;

        if (HAL_I2S_GetState(&pxHandle->handle) == HAL_I2S_STATE_RESET)
                return IOT_I2S_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_I2S_Receive(&pxHandle->handle, (uint16_t *)pvBuffer, xBytes/2, 1000);
        if (status != HAL_OK)
                return IOT_I2S_READ_FAILED;

        return IOT_I2S_SUCCESS;
}

int32_t iot_i2s_write_sync( IotI2SHandle_t const pxI2SPeripheral,
                            uint8_t * const pvBuffer,
                            size_t xBytes ) {
        if (pxI2SPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_I2S_INVALID_VALUE;

        I2SHandle_t *pxHandle = (I2SHandle_t *)pxI2SPeripheral;

        if (HAL_I2S_GetState(&pxHandle->handle) == HAL_I2S_STATE_RESET)
                return IOT_I2S_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_I2S_Transmit(&pxHandle->handle, (uint16_t *)pvBuffer, xBytes/2, 1000);
        if (status != HAL_OK)
                return IOT_I2S_WRITE_FAILED;

        return IOT_I2S_SUCCESS;
}

int32_t iot_i2s_close( IotI2SHandle_t const pxI2SPeripheral ) {
        if (pxI2SPeripheral == NULL)
                return IOT_I2S_INVALID_VALUE;
        I2SHandle_t *pxHandle = (I2SHandle_t *)pxI2SPeripheral;

        if (HAL_I2S_GetState(&pxHandle->handle) == HAL_I2S_STATE_RESET)
                return IOT_I2S_INVALID_VALUE;

        HAL_I2S_DeInit(&pxHandle->handle);

        extern volatile uint32_t g_pfnRamVectors[];
        g_pfnRamVectors[NVIC_USER_IRQ_OFFSET + pxHandle->irq] = 0x00;

        i2s_list = ll_remove(i2s_list, pxHandle);
        free( pxHandle );

        return IOT_I2S_SUCCESS;
}

int32_t iot_i2s_ioctl( IotI2SHandle_t const pxI2SPeripheral,
                       IotI2SIoctlRequest_t xI2SRequest,
                       void * const pvBuffer ) {
        if (pxI2SPeripheral == NULL || pvBuffer == NULL)
                return IOT_I2S_INVALID_VALUE;

        I2SHandle_t *pxHandle = (I2SHandle_t *)pxI2SPeripheral;

        if (pxHandle->handle.Instance == NULL)
                return IOT_I2S_INVALID_VALUE;

        switch (xI2SRequest) {
                case eI2SSetConfig: {
                        IotI2sIoctlConfig_t *config = (IotI2sIoctlConfig_t *)pvBuffer;
                        pxHandle->config = *config;

                        // TODO
                } break;
                case eI2SGetConfig: {
                        *(IotI2sIoctlConfig_t *)pvBuffer = pxHandle->config;
                } break;
                case eI2SGetBusState: {
                        HAL_I2S_StateTypeDef status = HAL_I2S_GetState(&pxHandle->handle);
                        // TODO reset and error states
                        if (status == HAL_I2S_STATE_READY)
                                *(IotI2SBusStatus_t *)pvBuffer = eI2SBusIdle;
                        else if (status == HAL_I2S_STATE_BUSY || status == HAL_I2S_STATE_BUSY_TX || status == HAL_I2S_STATE_BUSY_RX || status == HAL_I2S_STATE_BUSY_TX_RX)
                                *(IotI2SBusStatus_t *)pvBuffer = eI2SBusBusy;
                } break;
                case eI2SGetTxNoOfbytes: *(uint16_t *)pvBuffer = pxHandle->handle.TxXferSize; break;
                case eI2SGetRxNoOfbytes: *(uint16_t *)pvBuffer = pxHandle->handle.RxXferSize; break;
                default: return IOT_I2S_INVALID_VALUE;
        }

        return IOT_I2S_SUCCESS;
}

int32_t iot_i2s_cancel( IotI2SHandle_t const pxI2SPeripheral ) {
        if (pxI2SPeripheral == NULL)
                return IOT_I2C_INVALID_VALUE;
        I2SHandle_t *pxHandle = (I2SHandle_t *)pxI2SPeripheral;

        if (HAL_I2S_GetState(&pxHandle->handle) == HAL_I2S_STATE_RESET)
                return IOT_I2C_INVALID_VALUE;

        if (HAL_I2S_GetState(&pxHandle->handle) == HAL_I2S_STATE_READY)
                return IOT_I2C_NOTHING_TO_CANCEL;

        // TODO

        return IOT_I2C_SUCCESS;
}

#endif //HAL_I2S_MODULE_ENABLED

