# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Bootloader for the **ART-Pi board** (STM32H750XBHx, Cortex-M7). It initializes the QSPI peripheral to map an external W25Q64 flash into the microcontroller's address space at `0x90000000`, then jumps to an application stored there. Built on **RT-Thread v3.1.4** using GCC (`arm-none-eabi-`).

## Build & Flash

Requires `scons` (Python package) and `arm-none-eabi-gcc` in your PATH, or set `RTT_EXEC_PATH`.

```bash
# Build (output: rt-thread.elf)
scons

# Clean build artifacts
scons -c

# Alternatively, use RT-Thread Studio (Eclipse-based IDE)
```

RT-Thread Studio 自带的工具链在 `platform/env_released/env/tools/gnu_gcc/arm_gcc/mingw/bin/` 下。

## Output

- `rt-thread.elf` — 链接生成的 ELF 文件
- `output.map` — 链接映射文件
- `.o` 文件生成在源文件所在目录（未使用 variant_dir）

## Configuration

- **Kconfig** (`Kconfig` + `rtconfig.h`): Toggle RT-Thread kernel features, FinSH shell, and peripheral drivers.
- **Board config** (`drivers/board.h`): Pin assignments, clock source/freq, UART config, ROM/RAM layout.
- **STM32 HAL config** (`drivers/stm32h7xx_hal_conf.h`): Enable/disable HAL peripheral modules.
- **Toolchain** (`rtconfig.py`): Set `EXEC_PATH` or `RTT_EXEC_PATH` env var to point at the ARM GCC toolchain.
- **Linker script** (`linkscripts/STM32H750XBHx/link.lds`): ROM 128K @ 0x08000000, RAM 128K @ 0x20000000.

## Project Structure

| Path | Purpose |
|---|---|
| `applications/main.c` | Bootloader entry — jumps to app at `BSP_QSPI_ADDR_BASE` + 4 |
| `drivers/board.h` / `board.c` | BSP configuration (pins, clocks, heap boundaries) |
| `drivers/drv_qspi.c` | QSPI + W25Q64 init, quad-enable, memory-mapped mode, status register ops, FinSH commands |
| `drv_usart.c` / `uart_config.h` | UART4 console driver |
| `drv_clk.c` | System clock init: HSI → PLL → 180 MHz |
| `libraries/STM32H7xx_HAL_Driver/` | STM32H7 HAL peripheral drivers |
| `libraries/CMSIS/` | CMSIS core, device headers, startup code |
| `rt-thread/` | RT-Thread RTOS kernel (v3.1.4): scheduler, IPC, FinSH shell |
| `packages/` | RT-Thread online packages |
| `linkscripts/STM32H750XBHx/link.lds` | GNU ld script with `.isr_vector`, `.text`, `.data`, `.bss`, `.stack` sections |

## Key Architecture Details

- **Boot flow**: Reset → Startup code → `rt_hw_board_init()` → clock init → I/D-cache enable → UART init → QSPI init (memory-mapped) → `main()` → disable caches → stop SysTick → set MSP from QSPI vector table → jump.
- **QSPI memory-mapped mode**: After init, the external flash is readable directly at `0x90000000`. The bootloader reads the application's reset vector (`*(0x90000004)`) to find the entry point.
- **No heap** (`RT_USING_NOHEAP`): the bootloader does not use dynamic memory allocation.
- **FinSH shell** provides runtime commands: `w25q_get_unique_id`, `w25q_quad_enable/disable`, `w25q_sr_get/set`, `w25q_write_enable/disable`.
- **Initialization functions** are auto-called via `INIT_BOARD_EXPORT` / `INIT_PREV_EXPORT` macros which place function pointers into `.rti_fn` linker sections.
