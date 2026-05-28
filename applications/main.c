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
typedef void (*pFunction)(void);
pFunction JumpToApplication;
int main(void)
{

    LOG_D("Hello RT-Thread!");
    LOG_D("Hello Test Bootloader");

    // 跳转
    JumpToApplication = (pFunction)(*(__IO uint32_t *)(BSP_QSPI_ADDR_BASE + 4));
    if (JumpToApplication <= BSP_QSPI_ADDR_BASE){
        return RT_ERROR;
    }
    // 清除内核cache
    SCB_DisableDCache();
    SCB_DisableICache();

    // 把RTOS停了,防止多线程其他操作影响boot, 即停掉systick, 内核配置
    SysTick->CTRL = 0;
    __set_MSP(*(__IO uint32_t *)BSP_QSPI_ADDR_BASE);
    JumpToApplication();

    return RT_EOK;
}
