//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/9/26.
//

#include "board.h"

IotGpioHandle_t ld2;
IotGpioHandle_t b1;
IotUARTHandle_t uart2;
IotI2CHandle_t i2c1;

void Error_Handler(void)
{
        __disable_irq();
        while (1)
        {
        }
}

int BoardInit(void) {
        HAL_Init();

        ld2 = iot_gpio_open(0x05); // GPIOA GPIO_PIN_5
        IotGpioDirection_t dir = eGpioDirectionOutput;
        IotGpioOutputMode_t mode = eGpioPushPull;
        iot_gpio_ioctl(ld2, eSetGpioDirection, &dir);
        iot_gpio_ioctl(ld2, eSetGpioOutputMode, &mode);


        b1 = iot_gpio_open(0x2D); // GPIOC GPIO_PIN_13
        dir = eGpioDirectionInput;
        IotGpioInterrupt_t it = eGpioInterruptFalling;
        iot_gpio_ioctl(b1, eSetGpioDirection, &dir);
        iot_gpio_ioctl(b1, eSetGpioInterrupt, &it);

        uart2 = iot_uart_open(2);
        IotUARTConfig_t uartConfig = {
                .ulBaudrate = 115200,
                .xParity = eUartParityNone,
                .xStopbits = eUartStopBitsOne,
                .ucWordlength = 8,
                .ucFlowControl = 0
        };
        iot_uart_ioctl(uart2, eUartSetConfig, &uartConfig);

        i2c1 = iot_i2c_open(1);
        IotI2CConfig_t i2cConfig = {
                .ulMasterTimeout = 1000,
                .ulBusFreq = 400000
        };
        iot_i2c_ioctl(i2c1, eI2CSetMasterConfig, &i2cConfig);

/*
        I2C_HandleTypeDef hi2c1 = {0};
        hi2c1.Instance = I2C1;
        hi2c1.Mode = HAL_I2C_MODE_MASTER;
        hi2c1.Init.ClockSpeed = 400000;
        hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
        hi2c1.Init.OwnAddress1 = 0;
        hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
        hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
        hi2c1.Init.OwnAddress2 = 0;
        hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
        hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
        if (HAL_I2C_Init(&hi2c1) != HAL_OK)
        {
                Error_Handler();
        }
        uint8_t val = 0x1 << 7;
        HAL_StatusTypeDef status;
        HAL_Delay(2000);
        status = HAL_I2C_Mem_Write(&hi2c1, 0xD0, 0x6B, I2C_MEMADD_SIZE_8BIT, &val, 1, 1000);
        HAL_Delay(50);
        status = HAL_I2C_Mem_Read(&hi2c1, 0xD0, 0x75, I2C_MEMADD_SIZE_8BIT, (uint8_t *)&val, 1, 1000);
        (void) status;*/
        return 0;
}

void SystemClock_Config(void)
{
        RCC_OscInitTypeDef RCC_OscInitStruct = {0};
        RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

        /** Configure the main internal regulator output voltage
        */
        __HAL_RCC_PWR_CLK_ENABLE();
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

        /** Initializes the RCC Oscillators according to the specified parameters
        * in the RCC_OscInitTypeDef structure.
        */
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
        RCC_OscInitStruct.HSIState = RCC_HSI_ON;
        RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
        RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
        RCC_OscInitStruct.PLL.PLLM = 16;
        RCC_OscInitStruct.PLL.PLLN = 336;
        RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
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
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
        RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        {
                Error_Handler();
        }
}

void HAL_MspInit(void) {
        __HAL_RCC_SYSCFG_CLK_ENABLE();
        __HAL_RCC_PWR_CLK_ENABLE();

        SystemClock_Config();
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        if(huart->Instance==USART2)
        {
                __HAL_RCC_GPIOA_CLK_ENABLE();
                /**USART2 GPIO Configuration
                PA2     ------> USART2_TX
                PA3     ------> USART2_RX
                */
                GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
                GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
                GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
                HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
                __HAL_RCC_USART2_CLK_ENABLE();
        }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        if (hi2c->Instance==I2C1) {
                __HAL_RCC_GPIOB_CLK_ENABLE();
                GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
                GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
                GPIO_InitStruct.Pull = GPIO_NOPULL;
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
                GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
                __HAL_RCC_I2C1_CLK_ENABLE();
        }
}
