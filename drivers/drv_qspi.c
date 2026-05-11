/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-05-08     OALH       the first version
 */
#include <rtthread.h>
#include <board.h>

#define STM32_EOK RT_EOK
#define STM32_ERROR RT_ERROR

QSPI_HandleTypeDef hqspi;
void MX_QUADSPI_Init(void)
{
  // CR寄存器和DCR寄存器
  hqspi.Instance            = QUADSPI;
  hqspi.Init.ClockPrescaler = 1;
  hqspi.Init.FifoThreshold  = 4;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
  hqspi.Init.FlashSize      = POSITION_VAL(0X1000000)-1;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_5_CYCLE;
  hqspi.Init.ClockMode      = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID        = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash      = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {

  }
}

void HAL_QSPI_MspInit(QSPI_HandleTypeDef *hqspi) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hqspi->Instance==QUADSPI)
  {
  /* USER CODE BEGIN QUADSPI_MspInit 0 */

  /* USER CODE END QUADSPI_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_QSPI_CLK_ENABLE();

    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    /**QUADSPI GPIO Configuration
    PG6     ------> QUADSPI_BK1_NCS
    PF6     ------> QUADSPI_BK1_IO3
    PF7     ------> QUADSPI_BK1_IO2
    PF8     ------> QUADSPI_BK1_IO0
    PF10     ------> QUADSPI_CLK
    PF9     ------> QUADSPI_BK1_IO1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
  }
}

int stm32_hw_qspi_init(void) {
    // QSPI Init
    MX_QUADSPI_Init();
    // QSPI Config
    return STM32_EOK;
}

int rt_hw_qspi_init()
{
    stm32_hw_qspi_init();
    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_qspi_init);
