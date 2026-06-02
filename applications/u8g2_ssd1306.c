#include "rtconfig.h"
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <u8g2_port.h>

#define OLED_I2C_BUS   "i2c3"
#define SCREEN_W  128
#define SCREEN_H  64

static u8g2_t u8g2;
static rt_thread_t bounce_tid = RT_NULL;
static volatile rt_uint8_t bounce_running = 0;

/*
 * u8g2 I2C byte function using RT-Thread's I2C device "i2c3".
 *
 * CAD layer (u8x8_cad_ssd13xx_i2c) ALREADY inserts the 0x00/0x40 control
 * byte as a separate SEND call before each command/data.  So we just buffer
 * raw bytes and flush them at END_TRANSFER — DO NOT insert extra control
 * bytes ourselves or the data stream gets corrupted.
 */
static struct rt_i2c_bus_device *u8g2_i2c_bus = RT_NULL;
/* Buffer: max I2C payload for one full SSD1306 frame (~1040 bytes) */
#define U8G2_I2C_BUF_SIZE  1280
static uint8_t u8g2_i2c_buf[U8G2_I2C_BUF_SIZE];
static uint16_t u8g2_i2c_idx = 0;

static uint8_t u8x8_byte_rtthread_i2c3(u8x8_t *u8x8,
                                        uint8_t msg, uint8_t arg_int,
                                        void *arg_ptr)
{
    uint8_t *data;
    struct rt_i2c_msg msgs;

    switch (msg)
    {
    case U8X8_MSG_BYTE_INIT:
        u8g2_i2c_bus = rt_i2c_bus_device_find(OLED_I2C_BUS);
        if (u8g2_i2c_bus == RT_NULL)
            rt_kprintf("u8g2: can't find i2c bus \"%s\"\n", OLED_I2C_BUS);
        return (u8g2_i2c_bus != RT_NULL) ? 1 : 0;

    case U8X8_MSG_BYTE_SET_DC:
        /* CAD layer handles control bytes — nothing to do here */
        return 1;

    case U8X8_MSG_BYTE_START_TRANSFER:
        u8g2_i2c_idx = 0;
        return 1;

    case U8X8_MSG_BYTE_SEND:
        if (u8g2_i2c_idx + arg_int > U8G2_I2C_BUF_SIZE)
            return 0;
        data = (uint8_t *)arg_ptr;
        while (arg_int--)
            u8g2_i2c_buf[u8g2_i2c_idx++] = *data++;
        return 1;

    case U8X8_MSG_BYTE_END_TRANSFER:
        if (u8g2_i2c_idx == 0)
            return 0;
        msgs.addr  = u8x8_GetI2CAddress(u8x8) >> 1;
        msgs.flags = RT_I2C_WR;
        msgs.buf   = u8g2_i2c_buf;
        msgs.len   = u8g2_i2c_idx;
        if (rt_i2c_transfer(u8g2_i2c_bus, &msgs, 1) != 1)
        {
            rt_kprintf("u8g2: I2C xfer failed (addr=0x%02x, len=%d)\n",
                       msgs.addr, msgs.len);
            return 0;
        }
        return 1;

    default:
        return 0;
    }
}

/* ------------------------------------------------------------------ */
/*  Heart bitmap (16×11 px, fits wqy12 text height)                  */
/* ------------------------------------------------------------------ */
#define HEART_W  16
#define HEART_H  11
static const unsigned char heart_bits[] =
{
    0x3C, 0x3C,
    0x7E, 0x7E,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0x7F, 0xFE,
    0x3F, 0xFC,
    0x1F, 0xF8,
    0x0F, 0xF0,
    0x07, 0xE0,
};

/* ------------------------------------------------------------------ */
/*  Bounce thread                                                     */
/* ------------------------------------------------------------------ */

static void bounce_thread_entry(void *parameter)
{
    int x = 10, y = 12;             /* top-left corner (not baseline!) */
    int dx = 1, dy = 1;
    const char *s1 = "吴航";
    const char *s2 = "萱萱";

    u8g2_SetPowerSave(&u8g2, 0);

    while (bounce_running)
    {
        u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);
        int w1 = u8g2_GetUTF8Width(&u8g2, s1);          /* "吴航" */
        int w2 = u8g2_GetUTF8Width(&u8g2, s2);          /* "萱萱" */
        int total_w = w1 + HEART_W + w2;

        u8g2_ClearBuffer(&u8g2);

        /* "吴航" */
        u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);
        u8g2_DrawUTF8(&u8g2, x, y + 10, s1);            /* y+10 = baseline for 12px font */

        /* ❤ — hand-drawn bitmap, always works */
        u8g2_DrawXBM(&u8g2, x + w1, y, HEART_W, HEART_H, heart_bits);

        /* "萱萱" */
        u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);
        u8g2_DrawUTF8(&u8g2, x + w1 + HEART_W, y + 10, s2);

        u8g2_SendBuffer(&u8g2);

        x += dx;
        y += dy;

        if (x < 0)            { x = 0;        dx = -dx; }
        if (x + total_w >= SCREEN_W)
                              { x = SCREEN_W - total_w; dx = -dx; }
        if (y < 0)            { y = 0;        dy = -dy; }
        if (y + HEART_H >= SCREEN_H)
                              { y = SCREEN_H - HEART_H; dy = -dy; }

        rt_thread_mdelay(30);
    }

    u8g2_SetPowerSave(&u8g2, 1);
}

/* ------------------------------------------------------------------ */
/*  Commands                                                          */
/* ------------------------------------------------------------------ */

static void bounce_start(int argc, char *argv[])
{
    if (bounce_running)
    {
        rt_kprintf("bounce already running\n");
        return;
    }

    /* Suspend LVGL thread to stop it contending I2C */
    rt_thread_t lvgl_tid = rt_thread_find("LVGL");
    if (lvgl_tid)
        rt_thread_suspend(lvgl_tid);

    static int initialized = 0;
    if (!initialized)
    {
        u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0,
                                                u8x8_byte_rtthread_i2c3,
                                                u8x8_gpio_and_delay_rtthread);
        rt_kprintf("u8g2: initializing display...\n");
        u8g2_InitDisplay(&u8g2);
        rt_kprintf("u8g2: display init done\n");
        initialized = 1;
    }

    bounce_running = 1;
    bounce_tid = rt_thread_create("bounce",
                                   bounce_thread_entry, RT_NULL,
                                   2048,
                                   RT_THREAD_PRIORITY_MAX - 2, 20);
    if (bounce_tid != RT_NULL)
    {
        rt_thread_startup(bounce_tid);
        rt_kprintf("bounce started\n");
    }
    else
    {
        bounce_running = 0;
        rt_kprintf("bounce: failed to create thread\n");
    }
}
MSH_CMD_EXPORT(bounce_start, "start bouncing \"吴航❤萱萱\"");

static void bounce_stop(int argc, char *argv[])
{
    if (!bounce_running)
    {
        rt_kprintf("bounce not running\n");
        return;
    }

    bounce_running = 0;
    rt_thread_mdelay(50);
    bounce_tid = RT_NULL;

    /* Resume LVGL thread */
    rt_thread_t lvgl_tid = rt_thread_find("LVGL");
    if (lvgl_tid)
        rt_thread_resume(lvgl_tid);

    rt_kprintf("bounce stopped, display off\n");
}
MSH_CMD_EXPORT(bounce_stop, "stop bouncing and turn off display");
