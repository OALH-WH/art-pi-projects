/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-05-08     RT-Thread    first version
 */

#include <rtthread.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>
#include <board.h>
#include <w25qxx.h>

extern volatile rt_bool_t qspi_ready;

typedef void (*pFunction)(void);
pFunction JumpToApplication;
int main(void)
{

    LOG_D("Hello RT-Thread!");
    LOG_D("Hello Test Bootloader");

#if BSP_QSPI_USING_EXAMPLE
    LOG_D("QSPI example mode, not jumping to external flash");
    W25QXX_Init();
#if BSP_QSPI_USR_MEM_MAP
    W25Q_Memory_Mapped_Enable();
#endif
    qspi_ready = RT_TRUE;
#endif

    if (!qspi_ready) {
        LOG_E("QSPI init failed, cannot boot from external flash");
        return RT_ERROR;
    }

#if BSP_QSPI_USR_MEM_MAP
    // 先停 cache，再读 QSPI，确保读到的是 Flash 真实值而非 D-Cache 中的过期数据
    // Invalidate D-Cache first (discard without write-back) to prevent
    // SCB_DisableDCache's Clean+Invalidate from writing-back cached QSPI
    // data to the read-only memory-mapped region, which causes IMPRECISERR.
    SCB_InvalidateDCache();
    SCB_DisableDCache();
    SCB_DisableICache();

    // 跳转
    JumpToApplication = (pFunction)(*(__IO uint32_t *)(BSP_QSPI_ADDR_BASE + 4));
    uint32_t app_entry = (uint32_t)JumpToApplication;
    if ((app_entry <= BSP_QSPI_ADDR_BASE) ||
        (app_entry >= (BSP_QSPI_ADDR_BASE + 0x100000))) {
        LOG_D("Invalid application address: %p", JumpToApplication);
        return RT_ERROR;
    }
    // 把RTOS停了,防止多线程其他操作影响boot, 即停掉systick, 内核配置
    SysTick->CTRL = 0;
    __set_MSP(*(__IO uint32_t *)BSP_QSPI_ADDR_BASE);
    JumpToApplication();
#endif
    return RT_EOK;
}
