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

    // 把RTOS停了,防止多线程其他操作影响boot, 即停掉systick, 内核配置
    //SysTick->CTRL = 0;

    // 把中断关了, 防止修改VTOR的时候受到影响, 内核配置
    /*
    uint8_t i = 8;
    for (;i < 8; i++) {
        NVIC->ICER = 0xffff;
        NVIC->ICPR = 0xffff;
    }
    */

    // 清除内核cache
    SCB_DisableDCache();
    SCB_DisableICache();

    // 以上就是清理环境

    // 跳转
    JumpToApplication = (pFunction)(*(__IO uint32_t *)(BSP_QSPI_ADDR_BASE + 4));
    __set_MSP(*(__IO uint32_t *)BSP_QSPI_ADDR_BASE);
    LOG_D("addr=0x%8x", JumpToApplication);
    //JumpToApplication();

    /*
    int count = 1;
    uint8_t *pData = BSP_QSPI_ADDR_BASE;
    while (count++)
    {
        rt_thread_mdelay(1000);
        LOG_D("data=0x%x of 8-bits", *pData);
        pData++;

    }*/

    return RT_EOK;
}
