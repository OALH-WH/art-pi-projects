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
#include <rtdevice.h>
#include <lvgl.h>
#include "drv_ssd1306.h"

#ifdef RT_USING_I2C

#define BYTE_PER_PIXEL 2  /* LV_COLOR_DEPTH=16 → RGB565 = 2 bytes per pixel */

/* SSD1306 全屏帧缓冲（SSD1306 格式：页模式，每字节 8 个垂直像素） */
static uint8_t ssd1306_fb[SSD1306_WIDTH * SSD1306_PAGES];

/*
 * LVGL 每隔 N 行刷新的局部缓冲
 * 用 10 行缓冲避免栈溢出，同时节省内存
 */
#define ROW_BUF_CNT  10
static uint8_t disp_buf[SSD1306_WIDTH * ROW_BUF_CNT * BYTE_PER_PIXEL];

/**
 * 将 RGB565 像素转换为 1-bit 灰度（亮度阈值法）
 */
static inline uint8_t rgb565_to_1bit(uint16_t rgb565)
{
    /* 提取 R/G/B 分量 */
    uint8_t r = (rgb565 >> 11) & 0x1F;
    uint8_t g = (rgb565 >> 5)  & 0x3F;
    uint8_t b = rgb565         & 0x1F;

    /* 计算亮度（近似），R*0.3 + G*0.59 + B*0.11 */
    uint32_t lum = (r * 77 + g * 150 + b * 29) >> 5;
    return (lum > 127) ? 1 : 0;
}

/**
 * LVGL flush 回调：将 RGB565 渲染缓冲区渲染到 SSD1306
 */
static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{
    int32_t x, y;
    uint16_t *rgb565_buf = (uint16_t *)px_map;

    /* 将渲染区域转换为 SSD1306 页模式帧缓冲 */
    for (y = area->y1; y <= area->y2; y++)
    {
        for (x = area->x1; x <= area->x2; x++)
        {
            uint8_t pixel = rgb565_to_1bit(rgb565_buf[(y - area->y1) * (area->x2 - area->x1 + 1) + (x - area->x1)]);

            /* SSD1306 帧缓冲布局：每页 128 字节，每字节 8 个垂直像素 */
            uint32_t page  = y / 8;
            uint32_t row   = y % 8;
            uint32_t index = page * SSD1306_WIDTH + x;

            if (pixel)
                ssd1306_fb[index] |=  (1 << row);
            else
                ssd1306_fb[index] &= ~(1 << row);
        }
    }

    /* 将变化的区域写入 SSD1306 */
    uint32_t start_page = area->y1 / 8;
    uint32_t end_page   = area->y2 / 8;

    for (uint32_t p = start_page; p <= end_page; p++)
    {
        ssd1306_set_write_pos(area->x1, p);
        ssd1306_write_data(&ssd1306_fb[p * SSD1306_WIDTH + area->x1],
                           area->x2 - area->x1 + 1);
    }

    lv_display_flush_ready(disp_drv);
}

/**
 * LVGL 显示端口初始化
 */
void lv_port_disp_init(void)
{
    /* 初始化 SSD1306 */
    if (rt_hw_ssd1306_init(SSD1306_I2C_BUS_NAME) != RT_EOK)
    {
        rt_kprintf("lv_port_disp: ssd1306 init failed\n");
        return;
    }

    /* 清空帧缓冲并刷新 */
    rt_memset(ssd1306_fb, 0x00, sizeof(ssd1306_fb));

    for (uint8_t p = 0; p < SSD1306_PAGES; p++)
    {
        ssd1306_set_write_pos(0, p);
        ssd1306_write_data(ssd1306_fb, SSD1306_WIDTH);
    }

    /* 创建 LVGL 显示设备 */
    lv_display_t *disp = lv_display_create(SSD1306_WIDTH, SSD1306_HEIGHT);
    if (disp == NULL)
    {
        rt_kprintf("lv_port_disp: lv_display_create failed\n");
        return;
    }

    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_buffers(disp, disp_buf, NULL, sizeof(disp_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    rt_kprintf("lv_port_disp: SSD1306 LVGL display ready (%dx%d)\n",
               SSD1306_WIDTH, SSD1306_HEIGHT);
}

#endif /* RT_USING_I2C */
