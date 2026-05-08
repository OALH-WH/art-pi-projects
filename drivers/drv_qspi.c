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

int rt_hw_qspi_init()
{
    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_qspi_init);
