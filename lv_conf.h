/**
 * @file lv_conf.h
 * Configuration file for LVGL 9.5 on ART-Pi (STM32H750, RT-Thread)
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*
 * NOTE: RT-Thread specific settings (LV_USE_OS, LV_USE_STDLIB_*,
 * LV_ATTRIBUTE_MEM_ALIGN, LV_BIG_ENDIAN_SYSTEM, LV_ASSERT_HANDLER)
 * are provided by packages/LVGL-latest/env_support/rt-thread/lv_rt_thread_conf.h
 * which is included before this file. Do not redefine them here.
 */

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DEF_REFR_PERIOD  33
#define LV_DPI_DEF 130

/*====================
   RENDERING CONFIGURATION
 *====================*/
#define LV_DRAW_BUF_STRIDE_ALIGN    1
#define LV_DRAW_BUF_ALIGN           4
#define LV_DRAW_TRANSFORM_USE_MATRIX 0

#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW
    #define LV_DRAW_SW_SUPPORT_RGB565        1
    #define LV_DRAW_SW_SUPPORT_RGB565A8      1
    #define LV_DRAW_SW_SUPPORT_RGB888        1
    #define LV_DRAW_SW_SUPPORT_XRGB8888      1
    #define LV_DRAW_SW_SUPPORT_ARGB8888      1
    #define LV_DRAW_SW_SUPPORT_L8            1
    #define LV_DRAW_SW_SUPPORT_AL88          1
    #define LV_DRAW_SW_SUPPORT_A8            1
    #define LV_DRAW_SW_DRAW_UNIT_CNT    1
    #define LV_DRAW_SW_COMPLEX          1
    #if LV_DRAW_SW_COMPLEX
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
        #define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
    #endif
    #define LV_USE_DRAW_SW_ASM     LV_DRAW_SW_ASM_NONE
    #define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0
#endif

/* Use DMA2D hardware accelerator (STM32H7 has it) */
#define LV_USE_DRAW_DMA2D 0  /* set to 1 if DMA2D driver is implemented */

/*====================
   FONT USAGE
 *====================*/
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_FONT_FMT_TXT_LARGE 0
#define LV_USE_FONT_COMPRESSED 0
#define LV_USE_FONT_PLACEHOLDER 1

/*==================
   WIDGETS
 *==================*/
#define LV_WIDGETS_HAS_DEFAULT_VALUE  1

#define LV_USE_ANIMIMG      1
#define LV_USE_ARC          1
#define LV_USE_ARCLABEL     1
#define LV_USE_BAR          1
#define LV_USE_BUTTON       1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR     0
#define LV_USE_CANVAS       1
#define LV_USE_CHART        1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMAGE        1
#define LV_USE_IMAGEBUTTON  1
#define LV_USE_KEYBOARD     1
#define LV_USE_LABEL        1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 1
    #define LV_LABEL_LONG_TXT_HINT 1
    #define LV_LABEL_WAIT_CHAR_COUNT 3
#endif
#define LV_USE_LED          1
#define LV_USE_LINE         1
#define LV_USE_LIST         1
#define LV_USE_LOTTIE       0
#define LV_USE_MENU         1
#define LV_USE_MSGBOX       1
#define LV_USE_ROLLER       1
#define LV_USE_SCALE        1
#define LV_USE_SLIDER       1
#define LV_USE_SPAN         1
#define LV_USE_SPINBOX      1
#define LV_USE_SPINNER      1
#define LV_USE_SWITCH       1
#define LV_USE_TABLE        1
#define LV_USE_TABVIEW      1
#define LV_USE_TEXTAREA     1
#define LV_USE_TILEVIEW     1
#define LV_USE_WIN          1

/*==================
   THEMES
 *==================*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 0
    #define LV_THEME_DEFAULT_GROW 1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

#define LV_USE_THEME_SIMPLE 1
#define LV_USE_THEME_MONO   0

/*==================
   LAYOUTS
 *==================*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*====================
   TEXT SETTINGS
 *====================*/
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_)]}"
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#define LV_USE_BIDI 0
#define LV_USE_ARABIC_PERSIAN_CHARS 0
#define LV_TXT_COLOR_CMD "#"

/*====================
   LOGGING
 *====================*/
#define LV_USE_LOG 0

/*==================
   OTHERS
 *==================*/
#define LV_USE_OBSERVER 1
#define LV_USE_SNAPSHOT 0
#define LV_USE_SYSMON   0
#define LV_USE_PROFILER 0
#define LV_USE_MONKEY   0
#define LV_USE_GRIDNAV  0
#define LV_USE_FRAGMENT 0
#define LV_USE_IMGFONT  0
#define LV_USE_COLOR_FILTER 0

/*==================
   3RD PARTY LIBS
 *==================*/
#define LV_USE_LODEPNG 0
#define LV_USE_LIBPNG  0
#define LV_USE_TJPGD   0
#define LV_USE_LIBJPEG_TURBO 0
#define LV_USE_BMP     0
#define LV_USE_GIF     0
#define LV_USE_QRCODE  0
#define LV_USE_BARCODE 0
#define LV_USE_FREETYPE  0
#define LV_USE_RLOTTIE   0
#define LV_USE_FFMPEG    0
#define LV_USE_SVG       0

/*==================
   DEVICE DRIVERS
 *==================*/
#define LV_USE_SDL       0
#define LV_USE_X11       0
#define LV_USE_WAYLAND   0
#define LV_USE_LINUX_FBDEV   0
#define LV_USE_EVDEV     0
#define LV_USE_ST7735    0
#define LV_USE_ST7789    0
#define LV_USE_ST7796    0
#define LV_USE_ILI9341   0

/*==================
   COMPILER SETTINGS
 *==================*/
/* LV_BIG_ENDIAN_SYSTEM is set by lv_rt_thread_conf.h */
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1
/* LV_ATTRIBUTE_MEM_ALIGN is set by lv_rt_thread_conf.h */
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_FAST_MEM
#define LV_USE_FLOAT       0
#define LV_USE_MATRIX      0

/*==================
   BUILD OPTIONS
 *==================*/
#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS    0

#endif /* LV_CONF_H */
