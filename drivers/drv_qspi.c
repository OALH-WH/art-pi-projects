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

#define DBG_TAG "drv_qspi"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define STM32_EOK RT_EOK
#define STM32_ERROR RT_ERROR

QSPI_HandleTypeDef hqspi;
void MX_QUADSPI_Init(void)
{
  // CR寄存器和DCR寄存器
  hqspi.Instance            = QUADSPI;
  hqspi.Init.ClockPrescaler = 1;
  hqspi.Init.FifoThreshold  = 1;
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

typedef enum instruct_line {
    INVALID_LINE = 0,
    SINGLE_LINE = 0x10,
    DUAL_LINE = 0x20,
    QUAD_LINE = 0x30
}instruct_line_t;
static instruct_line_t cur_line = SINGLE_LINE;
int stm32_qspi_send_instruct(int instruct, instruct_mode_t instruct_mode, instruct_line_t instruct_line, int data_len, int dummyCycles) {
    QSPI_CommandTypeDef cmd = {0};
    cmd.Instruction = instruct;
    cmd.InstructionMode = QSPI_INSTRUCTION_NONE;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DummyCycles = 0;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    switch(instruct_mode | instruct_line) {
    case ONLY_INSTRCUT:
    case (ONLY_INSTRCUT | SINGLE_LINE):
        cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        break;
    case (ONLY_INSTRCUT | QUAD_LINE):
        cmd.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        break;
    case READ_INSTRUCT:
    case (READ_INSTRUCT | SINGLE_LINE):
        cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        cmd.DummyCycles = dummyCycles;
        cmd.DataMode = QSPI_DATA_1_LINE;
        cmd.NbData = data_len;
        break;
    default:
        LOG_D("not support flag=0x%2x", instruct_mode | instruct_line);
        return STM32_ERROR;
    }
    HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
    return STM32_EOK;
}


int stm32_qspi_receive_data(uint8_t *pData){
    HAL_QSPI_Receive(&hqspi, pData, HAL_MAX_DELAY);
    return STM32_EOK;
}
int w25q_exit_qpi_mode() {
    stm32_qspi_send_instruct(0xFF, ONLY_INSTRCUT, QUAD_LINE, 0, 0);
    return STM32_EOK;
}

void w25q_get_unique_id() {
    // 64-bits/8-bytes unique id
    uint8_t pData[8] = {0};
    uint64_t unique_id = 0;

    stm32_qspi_send_instruct(0x4B, READ_INSTRUCT, INVALID_LINE, 8, 31);
    stm32_qspi_receive_data(pData);
    for(int i=0; i<8; i++) {
        unique_id |= (pData[i] << (64 - 8 * (i + 1)));
    }
    LOG_D("unique_id=%u", unique_id);
}
MSH_CMD_EXPORT(w25q_get_unique_id, get w25q unique id);

void w25q_quad_enable() {

}

void w25q_quad_disable() {

}


typedef enum sr_num {
    W25QXX_SR_1 = 0x05,
    W25QXX_SR_2 = 0x35,
    W25QXX_SR_3 = 0x15
}sr_num_t;
uint32_t w25q_read_sr(sr_num_t sr_num) {
    uint8_t pData[1] = {0};
    uint8_t sr = 0;


    stm32_qspi_send_instruct(sr_num, READ_INSTRUCT, cur_line, 1, 0);
    stm32_qspi_receive_data(pData);
    for(int i=0; i<1;i++) {
        sr |= (pData[i] << (8 - 8 * (i + 1)));
    }
    return sr;
}

void w25q_get_sr(int argc, char **argv) {
    if (argc <= 1) {
        LOG_D("require sr num, eg. w25q_get_sr 1");
        return;
    }
    uint8_t sr=0;
    LOG_D("cmd=%s arg1=%c", argv[0], argv[1][0]);
    switch ((uint32_t)argv[1][0]) {
    case '1':
        sr=w25q_read_sr(W25QXX_SR_1);
        break;
    case '2':
        sr=w25q_read_sr(W25QXX_SR_2);
                break;
    case '3':
    sr=w25q_read_sr(W25QXX_SR_3);
            break;
    }
    LOG_D("sr=0x%x", sr);
}
MSH_CMD_EXPORT(w25q_get_sr, get w25qxx sr data);

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
