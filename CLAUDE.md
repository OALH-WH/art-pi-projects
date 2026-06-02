# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**ART-Pi blink_led** — an RT-Thread project for the **STM32H750XBHx** (Cortex-M7) on the ART-Pi board. It drives the onboard blue LED (PI8) and has **LVGL 9.5** integrated as an online package. The app loads from external QSPI flash at `0x90000000`.

## Build

Requires `arm-none-eabi-gcc` (GNU Arm Embedded Toolchain 10.3-2021.10), `scons` (SCons v4.10.1), and Python 3.14.

The toolchain path is set via the `RTT_EXEC_PATH` environment variable (RT-Thread Studio's bundled toolchain):
```
C:\Users\OALH\MyFiles\IDE\RT-ThreadStudio\platform\env_released\env\tools\gnu_gcc\arm_gcc\mingw\bin
```

```bash
scons        # Build rt-thread.elf + rtthread.bin + compile_commands.json
scons -c     # Clean build artifacts
```

**Known issue on this Windows setup:** The ARM GCC linker (`collect2.exe`) fails with `CreateProcess: No such file or directory` when spawned from the scons/Python process. The [SConstruct](./SConstruct) works around this by intercepting the link step and running `gcc` for linking in a **child Python process**, which has a clean process state where the toolchain works correctly. This is handled transparently — no special flags are needed.

### clangd Support

`compile_commands.json` is generated automatically during every build (see `CompilationDatabase` in SConstruct). The [.clangd](./.clangd) file at the project root points clangd to the compilation database for accurate code navigation and IntelliSense.

### menuconfig (RT-Thread Configuration)

RT-Thread is configured via Kconfig (see [Kconfig](./Kconfig) and `rtconfig.h`). Use RT-Thread Studio's **Env Tools** or the command line from the project root:

```bash
# Launch menuconfig UI (requires RT-Thread env tools)
C:\Users\OALH\MyFiles\IDE\RT-ThreadStudio\platform\env_released\env\tools\scripts\menuconfig.py

# After changing selections, regenerate rtconfig.h:
scons --pyconfig

# Update online packages (e.g. LVGL):
pkgs --update       # If RT-Thread env is in PATH
```

`rtconfig.h` is auto-generated — do not edit it manually. Change settings via `menuconfig` or by toggling defines in the Kconfig system.

## Project Structure

| Path | Purpose |
|---|---|
| `applications/main.c` | App entry — blinks LED PI8 at 500ms interval, relocates VTOR to `QSPI_BASE` via `INIT_BOARD_EXPORT` |
| `applications/SConscript` | Build script for app source files |
| `board/board.h` | MCU config: clock (480 MHz HSE), UART4 pinout, ROM/RAM layout (ROM @ 0x90000000, RAM @ 0x24000000) |
| `board/board.c` | BSP initialization, heap boundaries |
| `board/linker_scripts/STM32H750XBHx/link.lds` | Linker script — text in QFlash, data/BSS in RAM, `.onchip_rom` section for bootloader |
| `rtconfig.h` | **Auto-generated** RT-Thread configuration (Kconfig output). Enables LVGL (`PKG_USING_LVGL`), FinSH, DFS, SPI, I2C, serial v1, pin driver |
| `rtconfig.py` | Toolchain selection (GCC/Keil/IAR), `DEVICE` flags (`-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard`), `POST_ACTION` (objcopy + size) |
| `Kconfig` | Top-level menuconfig, includes RT-Thread kernel, packages, and libraries Kconfig trees |

## LVGL Integration

LVGL is pulled in as an RT-Thread online package at `packages/LVGL-latest/` (v9.5, identified by `PKG_LVGL_VER_NUM 0x99999`). Key integration files:

- `packages/LVGL-latest/env_support/rt-thread/lv_rt_thread_port.c` — RT-Thread port: creates a dedicated `LVGL` thread that calls `lv_timer_handler()` at `PKG_LVGL_DISP_REFR_PERIOD` (5 ms). Calls `lv_port_disp_init()`, `lv_port_indev_init()`, `lv_user_gui_init()` — these must be provided externally.
- `packages/LVGL-latest/env_support/rt-thread/SConscript` — Build script that walks `src/` recursively, adding all `.c` files and include paths. Uses `LOCAL_CFLAGS = ' -std=c99'` for GCC.

**`lv_conf.h`** is at the [project root](./lv_conf.h) — a customized configuration for the ART-Pi board with RT-Thread. RT-Thread specific settings (`LV_USE_OS`, `LV_USE_STDLIB_*`, `LV_ATTRIBUTE_MEM_ALIGN`, etc.) are provided by `packages/LVGL-latest/env_support/rt-thread/lv_rt_thread_conf.h`, which is included before `lv_conf.h` and takes precedence. The project root is added to include paths in [SConstruct](./SConstruct#L48) so the compiler can find `lv_conf.h`.

To enable LVGL demos or examples, set `PKG_LVGL_USING_DEMOS` or `PKG_LVGL_USING_EXAMPLES` in `rtconfig.h`.

## Memory Layout

| Region | Start | Size | Contents |
|---|---|---|---|
| ROM (on-chip) | `0x08000000` | 8 MB | Bootloader image (`.onchip_rom` section) |
| QFlash | `0x90000000` | 8 MB | Application `.text`, `.rodata`, `.ARM.exidx` (QSPI memory-mapped) |
| RAM | `0x24000000` | 512 KB | `.data`, `.bss`, `.stack` (ITCM-RAM, AXI-SRAM via D1 domain) |

The application vector table is relocated to `0x90000000` by `vtor_config()` in `main.c` (called via `INIT_BOARD_EXPORT`).

## Key Dependencies

- **RT-Thread v4.1.0** (`RT_VER_NUM 0x40100`) — kernel in `rt-thread/`
- **STM32H7 HAL** — in `libraries/STM32H7xx_HAL/`
- **LVGL 9.5** — in `packages/LVGL-latest/`
- **Board drivers** — in `libraries/drivers/` (UART4, SPI, GPIO, SDIO, SDRAM, LCD, WLAN)

## Debug

- UART4 console at PA0 (TX) / PI9 (RX), 115200 baud (default RT-Thread FinSH)
- FinSH shell enabled with history, symbol table, and built-in commands
- LVGL log level set by `DBG_LVL` in `lv_rt_thread_port.c` (default: `DBG_INFO`)
