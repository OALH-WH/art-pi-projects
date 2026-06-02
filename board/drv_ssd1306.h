/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-02     wuhang       first version
 */

#ifndef __DRV_SSD1306_H__
#define __DRV_SSD1306_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64
#define SSD1306_PAGES           (SSD1306_HEIGHT / 8)

#define SSD1306_I2C_BUS_NAME    "i2c3"

/* I2C 7-bit address (typical for most SSD1306 modules) */
#define SSD1306_I2C_ADDR        0x3C

int  rt_hw_ssd1306_init(const char *i2c_bus_name);
void ssd1306_set_write_pos(uint8_t x, uint8_t page);
void ssd1306_write_data(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_SSD1306_H__ */
