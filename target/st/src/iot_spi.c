//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/12/26.
//

#include "iot_spi.h"

#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "linked_list.h"

typedef struct IotSPIDescriptor {
        SPI_HandleTypeDef handle;
        IRQn_Type irq;
        IotSPIMasterConfig_t config;
        IotSPICallback_t callback;
        void * pvUserContext;
} SPIHandle_t;

linked_list_t *spi_list = NULL;

void prv_iot_spi_irq() {
        const uint32_t irq = __get_IPSR() - NVIC_USER_IRQ_OFFSET;
        const linked_list_t *iter = spi_list;

        while (iter != NULL) {
                SPIHandle_t *ptr = (SPIHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (ptr->irq == irq) HAL_SPI_IRQHandler(&ptr->handle);
                        return;
                }
                iter = iter->next;
        }
}

void prv_iot_spi_callback(SPI_HandleTypeDef *handle, IotSPITransactionStatus_t xStatus) {
        const linked_list_t *iter = spi_list;

        while (iter != NULL) {
                const SPIHandle_t *ptr = (SPIHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (&ptr->handle == handle && ptr->callback != NULL) ptr->callback(xStatus, ptr->pvUserContext);
                        return;
                }
                iter = iter->next;
        }
}

void prv_iot_spi_transfer_completed_callback(SPI_HandleTypeDef *hspi) {
        prv_iot_spi_callback(hspi, eSPISuccess);
}

void prv_iot_spi_error_callback(SPI_HandleTypeDef *hspi) {
        if (hspi->State == HAL_SPI_STATE_BUSY_TX)
                prv_iot_spi_callback(hspi, eSPIWriteError);
        else if (hspi->State == HAL_SPI_STATE_BUSY_RX)
                prv_iot_spi_callback(hspi, eSPIReadError);
        else if (hspi->State == HAL_SPI_STATE_BUSY_TX_RX)
                prv_iot_spi_callback(hspi, eSPITransferError);
}


IotSPIHandle_t iot_spi_open( int32_t lSPIInstance ) {
        SPIHandle_t *pxHandle = malloc(sizeof(SPIHandle_t));
        memset(pxHandle, 0, sizeof(SPIHandle_t));
        SPI_TypeDef *instance = NULL;

        if (pxHandle == NULL) {
                return NULL;
        }

        switch (lSPIInstance) {
                case 1: instance = SPI1; pxHandle->irq = SPI1_IRQn; break;
                case 2: instance = SPI2; pxHandle->irq = SPI2_IRQn; break;
                case 3: instance = SPI3; pxHandle->irq = SPI3_IRQn; break;
                default: free( pxHandle ); return NULL;
        }

        const linked_list_t *iter = spi_list;

        while (iter != NULL) {
                const SPIHandle_t *ptr = (SPIHandle_t *)iter->ptr;
                if (ptr != NULL) {
                        if (ptr->handle.Instance == instance) // already registered
                                return (IotSPIHandle_t)ptr;
                }
                iter = iter->next;
        }
        extern volatile uint32_t g_pfnRamVectors[];
        g_pfnRamVectors[NVIC_USER_IRQ_OFFSET + pxHandle->irq] = (uint32_t)((uint32_t *)prv_iot_spi_irq);


        pxHandle->handle.Instance = instance;
        pxHandle->handle.Init.Mode = SPI_MODE_MASTER;
        pxHandle->handle.Init.Direction = SPI_DIRECTION_2LINES;
        pxHandle->handle.Init.DataSize = SPI_DATASIZE_8BIT;
        pxHandle->handle.Init.CLKPolarity = SPI_POLARITY_LOW;
        pxHandle->handle.Init.CLKPhase = SPI_PHASE_1EDGE;
        pxHandle->handle.Init.NSS = SPI_NSS_SOFT;
        pxHandle->handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
        pxHandle->handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
        pxHandle->handle.Init.TIMode = SPI_TIMODE_DISABLE;
        pxHandle->handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
        pxHandle->handle.Init.CRCPolynomial = 10;

        pxHandle->handle.RxCpltCallback = prv_iot_spi_transfer_completed_callback;
        pxHandle->handle.TxCpltCallback = prv_iot_spi_transfer_completed_callback;
        pxHandle->handle.TxRxCpltCallback = prv_iot_spi_transfer_completed_callback;
        pxHandle->handle.ErrorCallback = prv_iot_spi_error_callback;

        HAL_NVIC_SetPriority(pxHandle->irq, 0, 0);
        HAL_NVIC_EnableIRQ(pxHandle->irq);

        spi_list = ll_append(spi_list, pxHandle);

        return (IotSPIHandle_t)pxHandle;
}

void iot_spi_set_callback( IotSPIHandle_t const pxSPIPeripheral,
                           IotSPICallback_t xCallback,
                           void * pvUserContext ) {
        if (pxSPIPeripheral == NULL)
                return;

        SPIHandle_t *pxHandle = (SPIHandle_t *)pxSPIPeripheral;
        pxHandle->callback = xCallback;
        pxHandle->pvUserContext = pvUserContext;
}

int32_t iot_spi_ioctl( IotSPIHandle_t const pxSPIPeripheral,
                       IotSPIIoctlRequest_t xSPIRequest,
                       void * const pvBuffer ) {
        if (pxSPIPeripheral == NULL || pvBuffer == NULL)
                return IOT_SPI_INVALID_VALUE;

        SPIHandle_t *pxHandle = (SPIHandle_t *)pxSPIPeripheral;

        if (pxHandle->handle.Instance == NULL)
                return IOT_SPI_INVALID_VALUE;

        switch (xSPIRequest) {
                case eSPISetMasterConfig: {
                        const IotSPIMasterConfig_t *config = (IotSPIMasterConfig_t *)pvBuffer;
                        pxHandle->config = *config;

                        uint32_t pclk = 0;
                        if (pxHandle->handle.Instance == SPI1)
                                pclk = HAL_RCC_GetPCLK2Freq();
                        else pclk = HAL_RCC_GetPCLK1Freq();


                        if (config->ulFreq > pclk/2)
                                return IOT_SPI_INVALID_VALUE;

                        uint32_t prescaler = pclk/config->ulFreq;
                        prescaler % 2 == 0 ? :prescaler--;


                        switch (prescaler) {
                                case 2: pxHandle->handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2; break;
                                case 4: pxHandle->handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; break;
                                case 8: pxHandle->handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; break;
                                case 16: pxHandle->handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; break;
                                case 32: pxHandle->handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32; break;
                                case 64: pxHandle->handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64; break;
                                case 128: pxHandle->handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128; break;
                                case 256: pxHandle->handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256; break;
                                default: return IOT_SPI_INVALID_VALUE;
                        }

                        if (config->eMode == eSPIMode0 || config->eMode == eSPIMode1)
                                pxHandle->handle.Init.CLKPolarity = SPI_POLARITY_LOW;
                        else if (config->eMode == eSPIMode2 || config->eMode == eSPIMode3)
                                pxHandle->handle.Init.CLKPolarity = SPI_POLARITY_HIGH;
                        else return IOT_SPI_INVALID_VALUE;

                        if (config->eMode == eSPIMode0 || config->eMode == eSPIMode2)
                                pxHandle->handle.Init.CLKPhase = SPI_PHASE_1EDGE;
                        else if (config->eMode == eSPIMode1 || config->eMode == eSPIMode3)
                                pxHandle->handle.Init.CLKPhase = SPI_PHASE_2EDGE;
                        else return IOT_SPI_INVALID_VALUE;

                        if (config->eSetBitOrder == eSPIMSBFirst)
                                pxHandle->handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
                        else if (config->eSetBitOrder == eSPILSBFirst)
                                pxHandle->handle.Init.FirstBit = SPI_FIRSTBIT_LSB;
                        else return IOT_SPI_INVALID_VALUE;
                } break;
                case eSPIGetMasterConfig: {
                        IotSPIMasterConfig_t *config = (IotSPIMasterConfig_t *)pvBuffer;
                        *config = pxHandle->config;
                } break;
                case eSPIGetTxNoOfbytes: *(uint16_t *)pvBuffer = pxHandle->handle.TxXferSize; break;
                case eSPIGetRxNoOfbytes: *(uint16_t *)pvBuffer = pxHandle->handle.RxXferSize; break;
                default: return IOT_SPI_INVALID_VALUE;
        }

        return IOT_SPI_SUCCESS;
}

int32_t iot_spi_read_sync( IotSPIHandle_t const pxSPIPeripheral,
                           uint8_t * const pvBuffer,
                           size_t xBytes ) {
        if (pxSPIPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_SPI_INVALID_VALUE;

        SPIHandle_t *pxHandle = (SPIHandle_t *)pxSPIPeripheral;

        if (HAL_SPI_GetState(&pxHandle->handle) == HAL_SPI_STATE_RESET)
                return IOT_SPI_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_SPI_Receive(&pxHandle->handle, pvBuffer, xBytes, 1000);
        if (status != HAL_OK)
                return IOT_SPI_READ_FAILED;

        return IOT_SPI_SUCCESS;
}

int32_t iot_spi_read_async( IotSPIHandle_t const pxSPIPeripheral,
                            uint8_t * const pvBuffer,
                            size_t xBytes ) {
        if (pxSPIPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_SPI_INVALID_VALUE;

        SPIHandle_t *pxHandle = (SPIHandle_t *)pxSPIPeripheral;

        if (HAL_SPI_GetState(&pxHandle->handle) == HAL_SPI_STATE_RESET)
                return IOT_SPI_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_SPI_Receive_IT(&pxHandle->handle, pvBuffer, xBytes);
        if (status != HAL_OK)
                return IOT_SPI_READ_FAILED;

        return IOT_SPI_SUCCESS;
}

int32_t iot_spi_write_sync( IotSPIHandle_t const pxSPIPeripheral,
                           uint8_t * const pvBuffer,
                           size_t xBytes ) {
        if (pxSPIPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_SPI_INVALID_VALUE;

        SPIHandle_t *pxHandle = (SPIHandle_t *)pxSPIPeripheral;

        if (HAL_SPI_GetState(&pxHandle->handle) == HAL_SPI_STATE_RESET)
                return IOT_SPI_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_SPI_Transmit(&pxHandle->handle, pvBuffer, xBytes, 1000);
        if (status != HAL_OK)
                return IOT_SPI_READ_FAILED;

        return IOT_SPI_SUCCESS;
}

int32_t iot_spi_write_async( IotSPIHandle_t const pxSPIPeripheral,
                            uint8_t * const pvBuffer,
                            size_t xBytes ) {
        if (pxSPIPeripheral == NULL || pvBuffer == NULL || xBytes == 0)
                return IOT_SPI_INVALID_VALUE;

        SPIHandle_t *pxHandle = (SPIHandle_t *)pxSPIPeripheral;

        if (HAL_SPI_GetState(&pxHandle->handle) == HAL_SPI_STATE_RESET)
                return IOT_SPI_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_SPI_Transmit_IT(&pxHandle->handle, pvBuffer, xBytes);
        if (status != HAL_OK)
                return IOT_SPI_READ_FAILED;

        return IOT_SPI_SUCCESS;
}

int32_t iot_spi_transfer_sync( IotSPIHandle_t const pxSPIPeripheral,
                               uint8_t * const pvTxBuffer,
                               uint8_t * const pvRxBuffer,
                               size_t xBytes ) {
        if (pxSPIPeripheral == NULL || pvTxBuffer == NULL || pvRxBuffer == NULL || xBytes == 0)
                return IOT_SPI_INVALID_VALUE;

        SPIHandle_t *pxHandle = (SPIHandle_t *)pxSPIPeripheral;

        if (HAL_SPI_GetState(&pxHandle->handle) == HAL_SPI_STATE_RESET)
                return IOT_SPI_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&pxHandle->handle, pvTxBuffer, pvRxBuffer, xBytes, 1000);
        if (status != HAL_OK)
                return IOT_SPI_READ_FAILED;

        return IOT_SPI_SUCCESS;
}

int32_t iot_spi_transfer_async( IotSPIHandle_t const pxSPIPeripheral,
                               uint8_t * const pvTxBuffer,
                               uint8_t * const pvRxBuffer,
                               size_t xBytes ) {
        if (pxSPIPeripheral == NULL || pvTxBuffer == NULL || pvRxBuffer == NULL || xBytes == 0)
                return IOT_SPI_INVALID_VALUE;

        SPIHandle_t *pxHandle = (SPIHandle_t *)pxSPIPeripheral;

        if (HAL_SPI_GetState(&pxHandle->handle) == HAL_SPI_STATE_RESET)
                return IOT_SPI_INVALID_VALUE;

        HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_IT(&pxHandle->handle, pvTxBuffer, pvRxBuffer, xBytes);
        if (status != HAL_OK)
                return IOT_SPI_READ_FAILED;

        return IOT_SPI_SUCCESS;
}

