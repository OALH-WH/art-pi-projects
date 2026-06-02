/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-02     wuhang       first version
 */

#include <board.h>
#include <rtdevice.h>
#include "drv_ssd1306.h"

#ifdef RT_USING_I2C

#define DBG_TAG "drv.ssd1306"
#define DBG_LVL DBG_INFO
#include <drv_log.h>

static struct rt_i2c_bus_device *ssd1306_i2c_bus = RT_NULL;

/* SSD1306 commands */
#define SSD1306_CMD_MODE        0x00
#define SSD1306_DATA_MODE       0x40

#define SSD1306_SET_CONTRAST    0x81
#define SSD1306_DISPLAY_ON      0xAF
#define SSD1306_DISPLAY_OFF     0xAE
#define SSD1306_SET_MUX         0xA8
#define SSD1306_CHARGE_PUMP     0x8D
#define SSD1306_SET_START_LINE  0x40
#define SSD1306_SET_SEG_REMAP   0xA1
#define SSD1306_COM_SCAN_DEC    0xC8
#define SSD1306_SET_OFFSET      0xD3
#define SSD1306_SET_CLOCK_DIV   0xD5
#define SSD1306_SET_PRECHARGE   0xD9
#define SSD1306_SET_COM_PINS    0xDA
#define SSD1306_SET_VCOM_DESEL  0xDB
#define SSD1306_DISPLAY_ALL_ON  0xA4
#define SSD1306_DISPLAY_NORMAL  0xA6
#define SSD1306_DEACT_SCROLL    0x2E
#define SSD1306_SET_COL_ADDR    0x21
#define SSD1306_SET_PAGE_ADDR   0x22

static rt_err_t ssd1306_write_cmd(uint8_t cmd)
{
    struct rt_i2c_msg msgs[1];

    uint8_t buf[2] = { SSD1306_CMD_MODE, cmd };
    msgs[0].addr  = SSD1306_I2C_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len   = 2;
    msgs[0].buf   = buf;

    if (rt_i2c_transfer(ssd1306_i2c_bus, msgs, 1) != 1)
    {
        LOG_E("ssd1306 write cmd 0x%02x failed", cmd);
        return -RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t ssd1306_write_cmd2(uint8_t cmd, uint8_t arg)
{
    rt_err_t ret;
    ret = ssd1306_write_cmd(cmd);
    if (ret != RT_EOK) return ret;
    return ssd1306_write_cmd(arg);
}

/**
 * Initialize SSD1306 OLED via I2C
 */
int rt_hw_ssd1306_init(const char *i2c_bus_name)
{
    /* Find I2C bus */
    ssd1306_i2c_bus = rt_i2c_bus_device_find(i2c_bus_name);
    if (ssd1306_i2c_bus == RT_NULL)
    {
        LOG_E("ssd1306: can't find i2c bus '%s'", i2c_bus_name);
        return -RT_ERROR;
    }

    /* Init sequence */
    rt_thread_mdelay(100);               /* power-up delay */

    ssd1306_write_cmd(SSD1306_DISPLAY_OFF);

    ssd1306_write_cmd2(SSD1306_SET_CLOCK_DIV, 0x80);   /* clock div = 1, freq = default */
    ssd1306_write_cmd2(SSD1306_SET_MUX, 0x3F);          /* 64 MUX */
    ssd1306_write_cmd2(SSD1306_SET_OFFSET, 0x00);       /* no offset */
    ssd1306_write_cmd(SSD1306_SET_START_LINE);           /* start line = 0 */
    ssd1306_write_cmd2(SSD1306_CHARGE_PUMP, 0x14);      /* enable charge pump */
    ssd1306_write_cmd(SSD1306_SET_SEG_REMAP);           /* column 127 = SEG0 */
    ssd1306_write_cmd(SSD1306_COM_SCAN_DEC);            /* COM from N-1 to 0 */
    ssd1306_write_cmd2(SSD1306_SET_COM_PINS, 0x12);     /* alternative pin config */
    ssd1306_write_cmd2(SSD1306_SET_CONTRAST, 0xCF);
    ssd1306_write_cmd2(SSD1306_SET_PRECHARGE, 0xF1);
    ssd1306_write_cmd2(SSD1306_SET_VCOM_DESEL, 0x40);
    ssd1306_write_cmd(SSD1306_DISPLAY_ALL_ON);          /* follow RAM */
    ssd1306_write_cmd(SSD1306_DISPLAY_NORMAL);

    ssd1306_write_cmd(SSD1306_DEACT_SCROLL);
    ssd1306_write_cmd(SSD1306_DISPLAY_ON);

    LOG_I("ssd1306 initialized on %s", i2c_bus_name);
    return RT_EOK;
}

/**
 * Set column and page write position
 */
void ssd1306_set_write_pos(uint8_t x, uint8_t page)
{
    if (page >= SSD1306_PAGES) return;
    if (x >= SSD1306_WIDTH)    return;

    ssd1306_write_cmd2(SSD1306_SET_COL_ADDR, x);
    ssd1306_write_cmd(SSD1306_WIDTH - 1);
    ssd1306_write_cmd2(SSD1306_SET_PAGE_ADDR, page);
    ssd1306_write_cmd(SSD1306_PAGES - 1);
}

/**
 * Write data bytes to the SSD1306 at current write position
 */
void ssd1306_write_data(const uint8_t *data, uint16_t len)
{
    struct rt_i2c_msg msgs[1];

    /* Allocate on stack with reasonable limit */
    uint8_t *buf = rt_malloc(len + 1);
    if (buf == RT_NULL)
    {
        LOG_E("ssd1306: OOM for %d bytes", len + 1);
        return;
    }

    buf[0] = SSD1306_DATA_MODE;
    rt_memcpy(buf + 1, data, len);

    msgs[0].addr  = SSD1306_I2C_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len   = len + 1;
    msgs[0].buf   = buf;

    if (rt_i2c_transfer(ssd1306_i2c_bus, msgs, 1) != 1)
    {
        LOG_E("ssd1306 write data failed (len=%d)", len);
    }

    rt_free(buf);
}

#endif /* RT_USING_I2C */
