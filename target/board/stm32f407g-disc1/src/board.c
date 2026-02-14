//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/11/26.
//

#include "board.h"

IotGpioHandle_t ld3, ld4, ld5, ld6;
IotGpioHandle_t b1;
IotI2CHandle_t i2c1;
IotI2SHandle_t i2s3;
IotSPIHandle_t spi1;

void Error_Handler(void)
{
        __disable_irq();
        while (1)
        {
        }
}

int BoardInit(void) {
        HAL_Init();

        IotGpioDirection_t dir = eGpioDirectionOutput;
        IotGpioOutputMode_t mode = eGpioPushPull;

        ld3 = iot_gpio_open(0x3D); // GPIOD GPIO_PIN_13
        iot_gpio_ioctl(ld3, eSetGpioDirection, &dir);
        iot_gpio_ioctl(ld3, eSetGpioOutputMode, &mode);

        ld4 = iot_gpio_open(0x3C); // GPIOD GPIO_PIN_12
        iot_gpio_ioctl(ld4, eSetGpioDirection, &dir);
        iot_gpio_ioctl(ld4, eSetGpioOutputMode, &mode);

        ld5 = iot_gpio_open(0x3E); // GPIOD GPIO_PIN_14
        iot_gpio_ioctl(ld5, eSetGpioDirection, &dir);
        iot_gpio_ioctl(ld5, eSetGpioOutputMode, &mode);

        ld6 = iot_gpio_open(0x3F); // GPIOD GPIO_PIN_15
        iot_gpio_ioctl(ld6, eSetGpioDirection, &dir);
        iot_gpio_ioctl(ld6, eSetGpioOutputMode, &mode);

        b1 = iot_gpio_open(0x00); // GPIOA GPIO_PIN_0
        dir = eGpioDirectionInput;
        IotGpioInterrupt_t it = eGpioInterruptFalling;
        iot_gpio_ioctl(b1, eSetGpioDirection, &dir);
        iot_gpio_ioctl(b1, eSetGpioInterrupt, &it);

        i2c1 = iot_i2c_open(1);
        uint8_t i2c1_slave_addr = 0x94;
        IotI2CConfig_t i2cConfig = {
                .ulMasterTimeout = 1000,
                .ulBusFreq = 400000
        };
        iot_i2c_ioctl(i2c1, eI2CSetMasterConfig, &i2cConfig);
        iot_i2c_ioctl(i2c1, eI2CSetSlaveAddr, &i2c1_slave_addr);

        i2s3 = iot_i2s_open(3);

        spi1 = iot_spi_open(1);
        IotSPIMasterConfig_t spiConfig = {
                .ulFreq = 42000000,
                .eMode = eSPIMode0,
                .eSetBitOrder = eSPIMSBFirst
        };
        iot_spi_ioctl(spi1, eSPISetMasterConfig, &spiConfig);

        return 0;
}

void SystemClock_Config(void)
{
        RCC_OscInitTypeDef RCC_OscInitStruct = {0};
        RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

        /** Configure the main internal regulator output voltage
        */
        __HAL_RCC_PWR_CLK_ENABLE();
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

        /** Initializes the RCC Oscillators according to the specified parameters
        * in the RCC_OscInitTypeDef structure.
        */
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
        RCC_OscInitStruct.HSEState = RCC_HSE_ON;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
        RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
        RCC_OscInitStruct.PLL.PLLM = 8;
        RCC_OscInitStruct.PLL.PLLN = 336;
        RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
        RCC_OscInitStruct.PLL.PLLQ = 7;
        if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        {
                Error_Handler();
        }

        /** Initializes the CPU, AHB and APB buses clocks
        */
        RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                    |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
        RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
        RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
        {
                Error_Handler();
        }
}

void HAL_MspInit(void) {
        __HAL_RCC_SYSCFG_CLK_ENABLE();
        __HAL_RCC_PWR_CLK_ENABLE();

        SystemClock_Config();
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        if (hi2c->Instance==I2C1) {
                __HAL_RCC_GPIOB_CLK_ENABLE();
                /**I2C1 GPIO Configuration
                PB6     ------> I2C1_SCL
                PB9     ------> I2C1_SDA
                */
                GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_9;
                GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
                GPIO_InitStruct.Pull = GPIO_PULLUP;
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
                GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
                __HAL_RCC_I2C1_CLK_ENABLE();

                GPIO_InitStruct.Pin = GPIO_PIN_4;
                GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
                GPIO_InitStruct.Pull = GPIO_NOPULL;;
                GPIO_InitStruct.Alternate = 0;
                HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
        }
}

void HAL_I2S_MspInit(I2S_HandleTypeDef* hi2s)
{
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
        if(hi2s->Instance==SPI3)
        {
                PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
                PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
                PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
                if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
                {
                        Error_Handler();
                }

                __HAL_RCC_SPI3_CLK_ENABLE();

                __HAL_RCC_GPIOA_CLK_ENABLE();
                __HAL_RCC_GPIOC_CLK_ENABLE();
                /**I2S3 GPIO Configuration
                PA4     ------> I2S3_WS
                PC7     ------> I2S3_MCK
                PC10     ------> I2S3_CK
                PC12     ------> I2S3_SD
                */
                GPIO_InitStruct.Pin = GPIO_PIN_4;
                GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
                GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
                HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

                GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_10|GPIO_PIN_12;
                GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
                GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
                HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        if(hspi->Instance==SPI1)
        {
                __HAL_RCC_SPI1_CLK_ENABLE();

                __HAL_RCC_GPIOA_CLK_ENABLE();
                /**SPI1 GPIO Configuration
                PA5     ------> SPI1_SCK
                PA6     ------> SPI1_MISO
                PA7     ------> SPI1_MOSI
                */
                GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
                GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
                GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
                HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        }
}
