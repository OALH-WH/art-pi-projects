/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-02     RT-Thread    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include <lvgl.h>

#define LED_PIN GET_PIN(I, 8)

int main(void)
{
    rt_uint32_t count = 1;

    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);

    while(count++)
    {
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN, PIN_LOW);
    }
    return RT_EOK;
}

/**
 * LVGL 用户界面初始化（由 LVGL 线程在初始化完成后调用）
 */
void lv_user_gui_init(void)
{
    /* 创建一个全屏的 label */
    
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello World!");
    lv_obj_center(label);

    rt_kprintf("lv_user_gui: Hello World label created\n");
    
}

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);


