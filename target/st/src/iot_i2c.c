//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/11/26.
//

#include "iot_i2c.h"

#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "linked_list.h"

typedef struct IotI2CDescriptor {
        I2C_HandleTypeDef handle;
        IRQn_Type irq;
        IotI2CConfig_t config;
        IotI2CCallback_t callback;
        uint16_t *addr;
        void * pvUserContext;
} I2CHandle_t;

linked_list_t *i2c_list = NULL;

void prv_iot_i2c_ev_irq() {
        const uint32_t irq = __get_IPSR() - NVIC_USER_IRQ_OFFSET;
        const linked_list_t *iter = i2c_list;

        while (iter != NULL) {
                I2CHandle_t *ptr = (I2CHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (ptr->irq == irq) HAL_I2C_EV_IRQHandler(&ptr->handle);
                        return;
                }
                iter = iter->next;
        }
}

void prv_iot_i2c_er_irq() {
        const uint32_t e_irq = __get_IPSR() - NVIC_USER_IRQ_OFFSET - 1;
        const linked_list_t *iter = i2c_list;

        while (iter != NULL) {
                I2CHandle_t *ptr = (I2CHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (ptr->irq == e_irq) HAL_I2C_ER_IRQHandler(&ptr->handle);
                        return;
                }
                iter = iter->next;
        }
}

void prv_iot_i2c_callback(I2C_HandleTypeDef *handle, IotI2COperationStatus_t xOpStatus) {
        const linked_list_t *iter = i2c_list;

        while (iter != NULL) {
                const I2CHandle_t *ptr = (I2CHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (&ptr->handle == handle && ptr->callback != NULL) ptr->callback(xOpStatus, ptr->pvUserContext);
                        return;
                }
                iter = iter->next;
        }
}

void prv_iot_i2c_master_transfer_completed_callback(I2C_HandleTypeDef *hi2c) {
        prv_iot_i2c_callback(hi2c, eI2CCompleted);
}

void prv_iot_i2c_error_callback(I2C_HandleTypeDef *hi2c) {
        if(HAL_I2C_GetState(hi2c) == HAL_I2C_STATE_TIMEOUT)
                prv_iot_i2c_callback(hi2c, eI2CMasterTimeout);
        else prv_iot_i2c_callback(hi2c, eI2CDriverFailed);
        // TODO eI2CNackFromSlave
}

IotI2CHandle_t iot_i2c_open( int32_t lI2CInstance ) {
        I2CHandle_t *pxHandle = malloc(sizeof(I2CHandle_t));
        memset(pxHandle, 0, sizeof(I2CHandle_t));
        I2C_TypeDef *instance = NULL;

        if (pxHandle == NULL) {
                return NULL;
        }

        switch (lI2CInstance) {
                case 1: instance = I2C1; __HAL_RCC_I2C1_CLK_ENABLE(); pxHandle->irq = I2C1_EV_IRQn; break;
                case 2: instance = I2C2; __HAL_RCC_I2C2_CLK_ENABLE(); pxHandle->irq = I2C2_EV_IRQn; break;
#ifdef I2C3
                case 3: instance = I2C3; __HAL_RCC_I2C3_CLK_ENABLE(); pxHandle->irq = I2C3_EV_IRQn; break;
#endif
                default: free( pxHandle ); return NULL;
        }

        const linked_list_t *iter = i2c_list;

        while (iter != NULL) {
                const I2CHandle_t *ptr = (I2CHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (ptr->handle.Instance == instance) // already registered
                                return (IotI2CHandle_t)ptr;
                }
                iter = iter->next;
        }
        extern volatile uint32_t g_pfnRamVectors[];
        g_pfnRamVectors[NVIC_USER_IRQ_OFFSET + pxHandle->irq] = (uint32_t)((uint32_t *)prv_iot_i2c_ev_irq);
        g_pfnRamVectors[NVIC_USER_IRQ_OFFSET + pxHandle->irq + 1] = (uint32_t)((uint32_t *)prv_iot_i2c_er_irq);

        pxHandle->handle.Instance = instance;
        pxHandle->handle.Mode = HAL_I2C_MODE_MASTER;
        pxHandle->handle.Init.ClockSpeed = 100000;
        pxHandle->handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
        pxHandle->handle.Init.OwnAddress1 = 0;
        pxHandle->handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
        pxHandle->handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
        pxHandle->handle.Init.OwnAddress2 = 0;
        pxHandle->handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
        pxHandle->handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

        pxHandle->handle.MasterTxCpltCallback = prv_iot_i2c_master_transfer_completed_callback;
        pxHandle->handle.MasterRxCpltCallback = prv_iot_i2c_master_transfer_completed_callback;
        pxHandle->handle.ErrorCallback = prv_iot_i2c_error_callback;
        // TODO register irq callbacks
        // pxHandle->handle.ListenCpltCallback = NULL;

        HAL_NVIC_SetPriority(pxHandle->irq, 0, 0);
        HAL_NVIC_EnableIRQ(pxHandle->irq);
        HAL_NVIC_SetPriority(pxHandle->irq + 1, 0, 0);
        HAL_NVIC_EnableIRQ(pxHandle->irq + 1);

        i2c_list = ll_append(i2c_list, pxHandle);

        return (IotI2CHandle_t)pxHandle;
}

void iot_i2c_set_callback( IotI2CHandle_t const pxI2CPeripheral,
                           IotI2CCallback_t xCallback,
                           void * pvUserContext ) {
        if (pxI2CPeripheral == NULL) return;

        I2CHandle_t *pxHandle = (I2CHandle_t *)pxI2CPeripheral;
        pxHandle->callback = xCallback;
        pxHandle->pvUserContext = pvUserContext;
}

int32_t iot_i2c_read_sync( IotI2CHandle_t const pxI2CPeripheral,
                           uint8_t * const pucBuffer,
                           size_t xBytes ) {
        if (pxI2CPeripheral == NULL || pucBuffer == NULL || xBytes == 0)
                return IOT_I2C_INVALID_VALUE;

        I2CHandle_t *pxHandle = (I2CHandle_t *)pxI2CPeripheral;

        if (pxHandle->addr == NULL)
                return IOT_I2C_SLAVE_ADDRESS_NOT_SET;
        HAL_StatusTypeDef status = HAL_ERROR;
        status = HAL_I2C_Master_Receive(&pxHandle->handle, *pxHandle->addr, pucBuffer, xBytes, pxHandle->config.ulMasterTimeout);

        if (status == HAL_BUSY)
                return IOT_I2C_BUSY;

        if (status == HAL_TIMEOUT)
                return IOT_I2C_BUS_TIMEOUT;

        if (status == HAL_ERROR)
                return IOT_I2C_READ_FAILED;

        return IOT_I2C_SUCCESS;
}

int32_t iot_i2c_write_sync( IotI2CHandle_t const pxI2CPeripheral,
                            uint8_t * const pucBuffer,
                            size_t xBytes ) {
        if (pxI2CPeripheral == NULL || pucBuffer == NULL || xBytes == 0)
                return IOT_I2C_INVALID_VALUE;

        I2CHandle_t *pxHandle = (I2CHandle_t *)pxI2CPeripheral;

        if (pxHandle->addr == NULL)
                return IOT_I2C_SLAVE_ADDRESS_NOT_SET;
        HAL_StatusTypeDef status = HAL_ERROR;
        status = HAL_I2C_Master_Transmit(&pxHandle->handle, *pxHandle->addr, pucBuffer, xBytes, pxHandle->config.ulMasterTimeout);

        if (status == HAL_BUSY)
                return IOT_I2C_BUSY;

        if (status == HAL_TIMEOUT)
                return IOT_I2C_BUS_TIMEOUT;

        if (status == HAL_ERROR)
                return IOT_I2C_WRITE_FAILED;

        return IOT_I2C_SUCCESS;
}

int32_t iot_i2c_read_async( IotI2CHandle_t const pxI2CPeripheral,
                            uint8_t * const pucBuffer,
                            size_t xBytes ) {
        if (pxI2CPeripheral == NULL || pucBuffer == NULL || xBytes == 0)
                return IOT_I2C_INVALID_VALUE;

        I2CHandle_t *pxHandle = (I2CHandle_t *)pxI2CPeripheral;

        if (pxHandle->addr == NULL)
                return IOT_I2C_SLAVE_ADDRESS_NOT_SET;
        HAL_StatusTypeDef status = HAL_ERROR;
        status = HAL_I2C_Master_Receive_IT(&pxHandle->handle, *pxHandle->addr, pucBuffer, xBytes);

        if (status == HAL_BUSY)
                return IOT_I2C_BUSY;

        if (status == HAL_TIMEOUT)
                return IOT_I2C_BUS_TIMEOUT;

        if (status == HAL_ERROR)
                return IOT_I2C_READ_FAILED;

        return IOT_I2C_SUCCESS;
}

int32_t iot_i2c_write_async( IotI2CHandle_t const pxI2CPeripheral,
                             uint8_t * const pucBuffer,
                             size_t xBytes ) {
        if (pxI2CPeripheral == NULL || pucBuffer == NULL || xBytes == 0)
                return IOT_I2C_INVALID_VALUE;

        I2CHandle_t *pxHandle = (I2CHandle_t *)pxI2CPeripheral;

        if (pxHandle->addr == NULL)
                return IOT_I2C_SLAVE_ADDRESS_NOT_SET;
        HAL_StatusTypeDef status = HAL_ERROR;
        status = HAL_I2C_Master_Transmit_IT(&pxHandle->handle, *pxHandle->addr, pucBuffer, xBytes);

        if (status == HAL_BUSY)
                return IOT_I2C_BUSY;

        if (status == HAL_TIMEOUT)
                return IOT_I2C_BUS_TIMEOUT;

        if (status == HAL_ERROR)
                return IOT_I2C_WRITE_FAILED;

        return IOT_I2C_SUCCESS;
}

int32_t iot_i2c_ioctl( IotI2CHandle_t const pxI2CPeripheral,
                       IotI2CIoctlRequest_t xI2CRequest,
                       void * const pvBuffer ) {
        if (pxI2CPeripheral == NULL || pvBuffer == NULL)
                return IOT_I2C_INVALID_VALUE;

        I2CHandle_t *pxHandle = (I2CHandle_t *)pxI2CPeripheral;

        if (pxHandle->handle.Instance == NULL)
                return IOT_I2C_INVALID_VALUE;

        switch (xI2CRequest) {
                case eI2CSendNoStopFlag: return IOT_I2C_FUNCTION_NOT_SUPPORTED;
                case eI2CSetSlaveAddr: {
                        pxHandle->addr = malloc(sizeof(uint16_t));
                        *pxHandle->addr = *(uint16_t *)pvBuffer;
                } break;
                case eI2CSetMasterConfig: {
                        IotI2CConfig_t *config = (IotI2CConfig_t *)pvBuffer;
                        pxHandle->config = *config;

                        pxHandle->handle.Init.ClockSpeed = config->ulBusFreq;
                        if (HAL_I2C_Init(&pxHandle->handle) != HAL_OK) return IOT_I2C_INVALID_VALUE;
                } break;
                case eI2CGetMasterConfig: {
                        IotI2CConfig_t *config = (IotI2CConfig_t *)pvBuffer;
                        *config = pxHandle->config;
                } break;
                case eI2CGetBusState: {
                        HAL_I2C_StateTypeDef state = HAL_I2C_GetState(&pxHandle->handle);
                        if (state == HAL_I2C_STATE_BUSY || state == HAL_I2C_STATE_BUSY_TX || state == HAL_I2C_STATE_BUSY_RX)
                                *(uint16_t *)pvBuffer = eI2cBusBusy;
                        else
                                *(uint16_t *)pvBuffer = eI2CBusIdle;
                } break;
                case eI2CBusReset: {
                        HAL_I2C_DeInit(&pxHandle->handle);
                        HAL_I2C_Init(&pxHandle->handle);
                } break;
                case eI2CGetTxNoOfbytes:
                case eI2CGetRxNoOfbytes: {
                        *(uint16_t *)pvBuffer = pxHandle->handle.XferSize;
                } break;
                default: return IOT_I2C_INVALID_VALUE;
        }

        return IOT_I2C_SUCCESS;
}

int32_t iot_i2c_close( IotI2CHandle_t const pxI2CPeripheral ) {
        if (pxI2CPeripheral == NULL)
                return IOT_I2C_INVALID_VALUE;

        I2CHandle_t *pxHandle = (I2CHandle_t *)pxI2CPeripheral;

        if (HAL_I2C_GetState(&pxHandle->handle) == HAL_I2C_STATE_RESET)
                return IOT_I2C_INVALID_VALUE;

        HAL_I2C_DeInit(&pxHandle->handle);

        extern volatile uint32_t g_pfnRamVectors[];
        g_pfnRamVectors[NVIC_USER_IRQ_OFFSET + pxHandle->irq] = 0x00;
        g_pfnRamVectors[NVIC_USER_IRQ_OFFSET + pxHandle->irq + 1] = 0x00;

        i2c_list = ll_remove(i2c_list, pxHandle);
        free( pxHandle );

        return IOT_I2C_SUCCESS;
}

int32_t iot_i2c_cancel( IotI2CHandle_t const pxI2CPeripheral ) {
        if (pxI2CPeripheral == NULL)
                return IOT_I2C_INVALID_VALUE;

        I2CHandle_t *pxHandle = (I2CHandle_t *)pxI2CPeripheral;

        if (HAL_I2C_GetState(&pxHandle->handle) == HAL_I2C_STATE_RESET)
                return IOT_I2C_INVALID_VALUE;

        if (HAL_I2C_GetState(&pxHandle->handle) == HAL_I2C_STATE_READY)
                return IOT_I2C_NOTHING_TO_CANCEL;

        if (pxHandle->handle.Instance->CR2 & I2C_CR2_ITERREN || pxHandle->handle.Instance->CR2 & I2C_CR2_ITEVTEN) {
                HAL_I2C_Master_Abort_IT(&pxHandle->handle, *pxHandle->addr);
        }

        // TODO what will do if synchronized transfer

        return IOT_I2C_SUCCESS;
}