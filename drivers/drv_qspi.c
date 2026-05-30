/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-05-08     OALH       the first version
 * 2026-05-29     OALH       refactor with RT-Thread device framework
 */
#include <rtthread.h>
#include <board.h>

#define DBG_TAG "drv.qspi"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* RT-Thread device control commands for W25Q flash operations */
#define RT_DEVICE_CTRL_QSPI_SR_READ       (RT_DEVICE_CTRL_CONFIG + 1)
#define RT_DEVICE_CTRL_QSPI_SR_WRITE      (RT_DEVICE_CTRL_CONFIG + 2)
#define RT_DEVICE_CTRL_QSPI_QUAD_ENABLE   (RT_DEVICE_CTRL_CONFIG + 3)
#define RT_DEVICE_CTRL_QSPI_QUAD_DISABLE  (RT_DEVICE_CTRL_CONFIG + 4)
#define RT_DEVICE_CTRL_QSPI_UNIQUE_ID     (RT_DEVICE_CTRL_CONFIG + 5)

/* Static globals */
static QSPI_HandleTypeDef hqspi;
static struct rt_device qspi_device;

/* Memory-mapped mode readiness flag, checked by main() before accessing 0x90000000 */
volatile rt_bool_t qspi_ready = RT_FALSE;

/* Instruction mode type definitions */
typedef enum instruct_mode {
    ONLY_INSTRCUT = 0,
    READ_INSTRUCT,
    WRITE_INSTRUCT,
} instruct_mode_t;

/* Instruction line width type definitions */
typedef enum instruct_line {
    INVALID_LINE = 0,
    SINGLE_LINE = 0x10,
    DUAL_LINE = 0x20,
    QUAD_LINE = 0x30
} instruct_line_t;
static instruct_line_t cur_line = SINGLE_LINE;

/* W25Q status register number definitions */
typedef enum sr_num {
    W25QXX_READ_SR_1 = 0x05,
    W25QXX_READ_SR_2 = 0x35,
    W25QXX_READ_SR_3 = 0x15,
    W25QXX_READ_SR_END = 0x01,
    W25QXX_WRITE_SR_1 = W25QXX_READ_SR_END,
    W25QXX_WRITE_SR_2 = 0x31,
    W25QXX_WRITE_SR_3 = 0x11,
    W25QXX_WRITE_SR_END
} sr_num_t;

#define SR_SET     1
#define SR_UNSET   0
#define SR_CUSTOM  2
#define W25QXX_SR_QE  (uint8_t)(1 << 1)

/*===========================================================================*
 *                         HAL Callback (STM32 HAL)                         *
 *===========================================================================*/
void HAL_QSPI_MspInit(QSPI_HandleTypeDef *hqspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hqspi->Instance == QUADSPI)
    {
        /* Peripheral clock enable */
        __HAL_RCC_QSPI_CLK_ENABLE();
        __HAL_RCC_GPIOG_CLK_ENABLE();
        __HAL_RCC_GPIOF_CLK_ENABLE();

        /* PG6 ------> QUADSPI_BK1_NCS */
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

        /* PF6/PF7/PF10 ------> QUADSPI_BK1_IO3/IO2/CLK */
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

        /* PF8/PF9 ------> QUADSPI_BK1_IO0/IO1 */
        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    }
}

/*===========================================================================*
 *                      HAL-Level Primitives (stm32_ prefix)                *
 *===========================================================================*/
static int stm32_qspi_hw_init(void)
{
    hqspi.Instance            = QUADSPI;
    hqspi.Init.ClockPrescaler = 1;
    hqspi.Init.FifoThreshold  = 1;
    hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    hqspi.Init.FlashSize      = POSITION_VAL(0x1000000) - 1;
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_5_CYCLE;
    hqspi.Init.ClockMode      = QSPI_CLOCK_MODE_0;
    hqspi.Init.FlashID        = QSPI_FLASH_ID_1;
    hqspi.Init.DualFlash      = QSPI_DUALFLASH_DISABLE;
    if (HAL_QSPI_Init(&hqspi) != HAL_OK)
    {
        LOG_E("HAL_QSPI_Init failed");
        return -RT_ERROR;
    }
    return RT_EOK;
}

static int stm32_qspi_send_command(int instruct, instruct_mode_t instruct_mode,
                                   instruct_line_t instruct_line, int data_len, int dummyCycles)
{
    QSPI_CommandTypeDef cmd = {0};
    cmd.Instruction = instruct;
    cmd.InstructionMode = QSPI_INSTRUCTION_NONE;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DummyCycles = 0;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    switch (instruct_mode | instruct_line) {
    case ONLY_INSTRCUT:
    case (ONLY_INSTRCUT | SINGLE_LINE):
        cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        break;
    case (ONLY_INSTRCUT | QUAD_LINE):
        cmd.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        break;
    case READ_INSTRUCT:
    case (READ_INSTRUCT | SINGLE_LINE):
    case WRITE_INSTRUCT:
    case (WRITE_INSTRUCT | SINGLE_LINE):
        cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        cmd.DummyCycles = dummyCycles;
        cmd.DataMode = QSPI_DATA_1_LINE;
        cmd.NbData = data_len;
        break;
    default:
        LOG_D("not support flag=0x%2x", instruct_mode | instruct_line);
        return -RT_ERROR;
    }
    HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY);
    return RT_EOK;
}

static int stm32_qspi_receive_data(uint8_t *pData)
{
    HAL_QSPI_Receive(&hqspi, pData, HAL_MAX_DELAY);
    return RT_EOK;
}

/*===========================================================================*
 *                  W25Q Flash Protocol Operations (w25q_ prefix)            *
 *===========================================================================*/
void w25q_get_unique_id(void)
{
    uint8_t pData[8] = {0};
    uint64_t unique_id = 0;

    stm32_qspi_send_command(0x4B, READ_INSTRUCT, INVALID_LINE, 8, 31);
    stm32_qspi_receive_data(pData);
    for (int i = 0; i < 8; i++) {
        unique_id |= (pData[i] << (64 - 8 * (i + 1)));
    }
    LOG_D("unique_id=%u", unique_id);
}
MSH_CMD_EXPORT(w25q_get_unique_id, get w25q unique id);

static uint8_t w25q_sr_read(sr_num_t sr_num)
{
    uint8_t pData[1] = {0};
    uint8_t sr = 0;

    stm32_qspi_send_command(sr_num, READ_INSTRUCT, cur_line, 1, 0);
    stm32_qspi_receive_data(pData);
    for (int i = 0; i < 1; i++) {
        sr |= (pData[i] << (8 - 8 * (i + 1)));
    }
    return sr;
}

static void w25q_write_enable_for_volatile(void)
{
    stm32_qspi_send_command(0x50, ONLY_INSTRCUT, cur_line, 0, 0);
}

static uint32_t w25q_write_sr(sr_num_t sr_num, int set, uint8_t mask)
{
    uint8_t cur_sr = 0;
    uint8_t pData = 0;
    int srnum = 0;

    switch (sr_num) {
    case W25QXX_WRITE_SR_1:
        srnum = 1;
        cur_sr = w25q_sr_read(W25QXX_READ_SR_1);
        break;
    case W25QXX_WRITE_SR_2:
        srnum = 2;
        cur_sr = w25q_sr_read(W25QXX_READ_SR_2);
        break;
    case W25QXX_WRITE_SR_3:
        srnum = 3;
        cur_sr = w25q_sr_read(W25QXX_READ_SR_3);
        break;
    default:
        LOG_D("sr_num=%d is not supported", sr_num);
        return -RT_ERROR;
    }
    w25q_write_enable_for_volatile();
    stm32_qspi_send_command(sr_num, WRITE_INSTRUCT, cur_line, 1, 0);
    if (set == SR_SET) {
        pData = cur_sr | mask;
        LOG_D("sr set pData=0x%x", pData);
    } else if (set == SR_UNSET) {
        pData = cur_sr & (~mask);
        LOG_D("sr unset pData=0x%x", pData);
    } else {
        pData = mask;
        LOG_D("sr custom pData=0x%x", pData);
    }
    HAL_QSPI_Transmit(&hqspi, &pData, HAL_MAX_DELAY);
    LOG_D("sr[%d]=0x%x change to 0x%x", srnum, cur_sr, pData);
    return RT_EOK;
}

uint32_t w25q_write_enable(void)
{
    stm32_qspi_send_command(0x06, ONLY_INSTRCUT, cur_line, 0, 0);
    return RT_EOK;
}
MSH_CMD_EXPORT(w25q_write_enable, w25q write enable);

uint32_t w25q_write_disable(void)
{
    stm32_qspi_send_command(0x04, ONLY_INSTRCUT, cur_line, 0, 0);
    return RT_EOK;
}
MSH_CMD_EXPORT(w25q_write_disable, w25q write disable);

void w25q_quad_enable(void)
{
    w25q_write_sr(W25QXX_WRITE_SR_2, SR_SET, W25QXX_SR_QE);
}
MSH_CMD_EXPORT(w25q_quad_enable, w25qxx quad enable);

void w25q_quad_disable(void)
{
    w25q_write_sr(W25QXX_WRITE_SR_2, SR_UNSET, W25QXX_SR_QE);
}
MSH_CMD_EXPORT(w25q_quad_disable, w25qxx quad disable);

void w25q_sr_get(int argc, char **argv)
{
    if (argc <= 1) {
        LOG_D("require sr num, eg. w25q_get_sr 1");
        return;
    }
    uint8_t sr = 0;
    LOG_D("cmd=%s arg1=%c", argv[0], argv[1][0]);
    switch ((uint32_t)argv[1][0]) {
    case '1':
        sr = w25q_sr_read(W25QXX_READ_SR_1);
        break;
    case '2':
        sr = w25q_sr_read(W25QXX_READ_SR_2);
        break;
    case '3':
        sr = w25q_sr_read(W25QXX_READ_SR_3);
        break;
    }
    LOG_D("sr=0x%x", sr);
}
MSH_CMD_EXPORT(w25q_sr_get, get w25qxx sr data);

void w25q_sr_set(int argc, char **argv)
{
    if (argc <= 2) {
        LOG_D("require sr num and set/unset, eg. w25q_sr_reset 1 1 0xff");
        return;
    }
    LOG_D("cmd=%s arg1=%s arg2=%s arg3=%s", argv[0], argv[1], argv[2], argv[3]);
    uint8_t set = argv[2][0] == '1' ? SR_SET : \
                  (argv[2][0] == '0' ? SR_UNSET : SR_CUSTOM);
    LOG_D("set=%d", set);
    uint8_t mask = 0x00, str_ptr = 0;
    while (argv[3][str_ptr] != '\0') {
        if (str_ptr == 0 && argv[3][str_ptr] == '0') {
            str_ptr++;
            continue;
        }
        if (argv[3][str_ptr] == 'x' || argv[3][str_ptr] == 'X') {
            str_ptr++;
            continue;
        }
        mask <<= 4;
        if (argv[3][str_ptr] >= '0' && argv[3][str_ptr] <= '9') {
            mask |= (argv[3][str_ptr] - '0');
        } else if (argv[3][str_ptr] >= 'a' && argv[3][str_ptr] <= 'f') {
            mask |= (argv[3][str_ptr] - 'a' + 10);
        } else if (argv[3][str_ptr] >= 'A' && argv[3][str_ptr] <= 'F') {
            mask |= (argv[3][str_ptr] - 'A' + 10);
        } else {
            LOG_D("invalid mask char=%c", argv[3][str_ptr]);
            return;
        }
        str_ptr++;
    }
    switch ((uint32_t)argv[1][0]) {
    case '1':
        w25q_write_sr(W25QXX_WRITE_SR_1, set, mask);
        break;
    case '2':
        w25q_write_sr(W25QXX_WRITE_SR_2, set, mask);
        break;
    case '3':
        w25q_write_sr(W25QXX_WRITE_SR_3, set, mask);
        break;
    }
}
MSH_CMD_EXPORT(w25q_sr_set, status registe set eg. w25q_sr_reset 1 1 0xff);

/*===========================================================================*
 *                  Memory-Mapped Mode Entry                                 *
 *===========================================================================*/
static int stm32_qspi_enter_memory_mapped_mode(void)
{
    QSPI_CommandTypeDef cmd = {0};
    QSPI_MemoryMappedTypeDef cfg = {0};

    /* Ensure QE bit is set before entering quad mode */
    uint8_t enable_qe_count = 0;
    uint8_t sr;
check_qe:
    sr = w25q_sr_read(W25QXX_READ_SR_2);
    if (!(sr & 0b10)) {
        if (enable_qe_count > 3) {
            LOG_E("QE bit failed to set after %d attempts", enable_qe_count);
            return -RT_ERROR;
        }
        enable_qe_count++;
        LOG_D("QE bit not set, attempt %d", enable_qe_count);
        w25q_quad_enable();
        goto check_qe;
    }

    /* Quad I/O Fast Read (0xEB) */
    cmd.Instruction = 0xEB;
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = QSPI_ADDRESS_4_LINES;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    
    cmd.DataMode = QSPI_DATA_4_LINES;
    cmd.DummyCycles = 4;

    cmd.AlternateBytes = 0x00;
    cmd.AlternateBytesSize = 1;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;

    cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    cfg.TimeOutPeriod = 0x0;

    if (HAL_QSPI_MemoryMapped(&hqspi, &cmd, &cfg) != HAL_OK)
    {
        LOG_E("HAL_QSPI_MemoryMapped failed");
        return -RT_ERROR;
    }
    return RT_EOK;
}

/*===========================================================================*
 *                    RT-Thread Device Ops (stm32_ prefix)                   *
 *===========================================================================*/
static rt_err_t stm32_qspi_device_init(rt_device_t dev)
{
    RT_ASSERT(dev != RT_NULL);

    if (dev->flag & RT_DEVICE_FLAG_ACTIVATED)
        return RT_EOK;

    if (stm32_qspi_hw_init() != RT_EOK)
        return -RT_ERROR;
#if BSP_QSPI_USR_MEM_MAP
    if (stm32_qspi_enter_memory_mapped_mode() != RT_EOK)
        return -RT_ERROR;
#endif
    qspi_ready = RT_TRUE;
    return RT_EOK;
}

static rt_err_t stm32_qspi_device_open(rt_device_t dev, rt_uint16_t oflag)
{
    RT_ASSERT(dev != RT_NULL);

    if (oflag & RT_DEVICE_OFLAG_WRONLY) {
        LOG_D("QSPI device is read-only in memory-mapped mode");
        return -RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t stm32_qspi_device_close(rt_device_t dev)
{
    RT_ASSERT(dev != RT_NULL);

    return RT_EOK;
}

static rt_size_t stm32_qspi_device_read(rt_device_t dev, rt_off_t pos,
                                        void *buffer, rt_size_t size)
{
    RT_ASSERT(dev != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    if (!qspi_ready)
        return 0;

    rt_memcpy(buffer, (void *)(BSP_QSPI_ADDR_BASE + pos), size);
    return size;
}

static rt_size_t stm32_qspi_device_write(rt_device_t dev, rt_off_t pos,
                                         const void *buffer, rt_size_t size)
{
    RT_ASSERT(dev != RT_NULL);

    /* Write not supported in memory-mapped mode */
    return 0;
}

static rt_err_t stm32_qspi_device_control(rt_device_t dev, int cmd, void *args)
{
    RT_ASSERT(dev != RT_NULL);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_QSPI_SR_READ:
    {
        sr_num_t sr = *(sr_num_t *)args;
        *(uint8_t *)args = w25q_sr_read(sr);
        return RT_EOK;
    }
    case RT_DEVICE_CTRL_QSPI_SR_WRITE:
    {
        /* args layout: sr_num_t sr; int set; uint8_t mask */
        sr_num_t sr = ((sr_num_t *)args)[0];
        int set = ((int *)args)[1];
        uint8_t mask = ((uint8_t *)args)[2];
        return w25q_write_sr(sr, set, mask);
    }
    case RT_DEVICE_CTRL_QSPI_QUAD_ENABLE:
        w25q_quad_enable();
        return RT_EOK;
    case RT_DEVICE_CTRL_QSPI_QUAD_DISABLE:
        w25q_quad_disable();
        return RT_EOK;
    case RT_DEVICE_CTRL_QSPI_UNIQUE_ID:
        w25q_get_unique_id();
        return RT_EOK;
    default:
        return -RT_ENOSYS;
    }
}

/*===========================================================================*
 *                   Init + Registration                                     *
 *===========================================================================*/
int rt_hw_qspi_init(void)
{
    rt_memset(&qspi_device, 0, sizeof(qspi_device));
    qspi_device.type    = RT_Device_Class_Char;
    qspi_device.flag    = RT_DEVICE_FLAG_RDONLY;
    qspi_device.init    = stm32_qspi_device_init;
    qspi_device.open    = stm32_qspi_device_open;
    qspi_device.close   = stm32_qspi_device_close;
    qspi_device.read    = stm32_qspi_device_read;
    qspi_device.write   = stm32_qspi_device_write;
    qspi_device.control = stm32_qspi_device_control;

    rt_device_register(&qspi_device, "qspi", RT_DEVICE_FLAG_RDONLY);

    /* Must init immediately — bootloader constraint */
    return rt_device_init(&qspi_device);
}
#if !BSP_QSPI_USING_EXAMPLE
INIT_BOARD_EXPORT(rt_hw_qspi_init);
#endif

/*===========================================================================*
 *                    Boot-Time Smoke Test                                    *
 *===========================================================================*/
#if BSP_QSPI_USR_MEM_MAP
 int rt_hw_qspi_test(void)
{
    if (!qspi_ready) {
        LOG_E("QSPI not ready, cannot read application entry");
        return -RT_ERROR;
    }
    void (*JumpToApplication)(void) = (void (*)(void))(*(__IO uint32_t *)(BSP_QSPI_ADDR_BASE + 4));
    LOG_D("addr=0x%p", JumpToApplication);
    return RT_EOK;
}
INIT_PREV_EXPORT(rt_hw_qspi_test);
#endif /* BSP_QSPI_USR_MEM_MAP */
