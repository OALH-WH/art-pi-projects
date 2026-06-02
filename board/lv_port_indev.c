/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-05-30     wuhang       first version
 */

#include <board.h>
#include <lvgl.h>

/**
 * LVGL 输入设备初始化（SSD1306 无触摸，本函数为空）
 */
void lv_port_indev_init(void)
{
    /* SSD1306 OLED 无触摸 / 按键输入，保留为空 */
}
