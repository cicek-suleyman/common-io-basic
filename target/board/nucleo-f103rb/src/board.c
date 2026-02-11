//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/11/26.
//

#include "board.h"

IotGpioHandle_t b1;
IotUARTHandle_t uart2;
IotGpioHandle_t ld2;

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

        return 0;
}
void SystemClock_Config(void)
{
        RCC_OscInitTypeDef RCC_OscInitStruct = {0};
        RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

        /** Initializes the RCC Oscillators according to the specified parameters
        * in the RCC_OscInitTypeDef structure.
        */
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
        RCC_OscInitStruct.HSIState = RCC_HSI_ON;
        RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
        RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
        RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
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
        __HAL_RCC_AFIO_CLK_ENABLE();
        __HAL_RCC_PWR_CLK_ENABLE();
        __HAL_AFIO_REMAP_SWJ_NOJTAG();


        SystemClock_Config();
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
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
                GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
                HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
                __HAL_RCC_USART2_CLK_ENABLE();
        }
}
