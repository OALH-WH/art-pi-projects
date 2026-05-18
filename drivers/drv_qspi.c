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

typedef enum instruct_mode {
ONLY_INSTRCUT = 0,
READ_INSTRUCT,
WRITE_INSTRUCT,
}instruct_mode_t;
int stm32_qspi_common_CCR(QSPI_CommandTypeDef *cmd, instruct_mode_t instrcut_mode) {
    cmd->InstructionMode = QSPI_INSTRUCTION_NONE;
    cmd->AddressMode = QSPI_ADDRESS_NONE;
    cmd->AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd->DataMode = QSPI_DATA_NONE;
    cmd->DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd->DummyCycles = 0;
    switch(instrcut_mode) {
    case ONLY_INSTRCUT:
        cmd->InstructionMode = QSPI_INSTRUCTION_1_LINE;
        break;
    case READ_INSTRUCT:
        cmd->InstructionMode = QSPI_INSTRUCTION_1_LINE;
        cmd->DummyCycles = 31;
        cmd->DataMode = QSPI_DATA_1_LINE;
        break;
    default:
        break;
    }
    return STM32_EOK;
}

int stm32_qspi_send_instruct(int instruct) {
    QSPI_CommandTypeDef cmd;
    cmd.Instruction = instruct;
    stm32_qspi_common_CCR(&cmd, ONLY_INSTRCUT);
    HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
    return STM32_EOK;
}


int stm32_qspi_receive_data(){
    //HAL_QSPI_Receive(&hqspi, pData, HAL_MAX_DELAY);
    return STM32_EOK;
}
int w25q_exit_qpi_mode() {
    stm32_qspi_send_instruct(0xFF);
    return STM32_EOK;
}



int stm32_hw_qspi_init(void) {
    // QSPI Init
    MX_QUADSPI_Init();
    // QSPI Config
    w25q_exit_qpi_mode();
    return STM32_EOK;
}

int rt_hw_qspi_init()
{
    stm32_hw_qspi_init();
    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_qspi_init);
