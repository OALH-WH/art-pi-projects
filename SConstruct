import os
import sys
import rtconfig

RTT_ROOT = os.path.normpath(os.getcwd() + '/rt-thread')

sys.path = sys.path + [os.path.join(RTT_ROOT, 'tools')]
from building import *

TARGET = 'Debug/rt-thread.elf'

DefaultEnvironment(tools=[])
env = Environment(tools = ['mingw'],
    AS = rtconfig.AS, ASFLAGS = rtconfig.AFLAGS,
    CC = rtconfig.CC, CCFLAGS = rtconfig.CFLAGS,
    AR = rtconfig.AR, ARFLAGS = '-rc',
    CXX = rtconfig.CXX, CXXFLAGS = rtconfig.CXXFLAGS,
    LINK = rtconfig.LINK, LINKFLAGS = rtconfig.LFLAGS)
env.PrependENVPath('PATH', rtconfig.EXEC_PATH)

env.Tool('compilation_db')

env.AppendUnique(CPPDEFINES = [
    'STM32H750xx',
    'SOC_FAMILY_STM32',
    'SOC_SERIES_STM32H7',
    'USE_HAL_DRIVER',
])

env.AppendUnique(CPPPATH = [
    '.',
    'applications',
    'drivers',
    'libraries/CMSIS/Device/ST/STM32H7xx/Include',
    'libraries/CMSIS/Include',
    'libraries/STM32H7xx_HAL_Driver/Inc',
    'libraries/CMSIS/RTOS',
    'rt-thread/include',
    'rt-thread/libcpu/arm/cortex-m7',
    'rt-thread/libcpu/arm/common',
    'rt-thread/components/finsh',
])

SRC_C = [
    # BSP Applications
    'applications/main.c',
    # BSP Drivers
    'drivers/board.c',
    'drivers/drv_clk.c',
    'drivers/drv_common.c',
    'drivers/drv_qspi.c',
    'drivers/drv_usart.c',
    # RT-Thread Kernel
    'rt-thread/src/clock.c',
    'rt-thread/src/components.c',
    'rt-thread/src/cpu.c',
    'rt-thread/src/device.c',
    'rt-thread/src/idle.c',
    'rt-thread/src/ipc.c',
    'rt-thread/src/irq.c',
    'rt-thread/src/kservice.c',
    'rt-thread/src/object.c',
    'rt-thread/src/scheduler.c',
    'rt-thread/src/thread.c',
    'rt-thread/src/timer.c',
    # RT-Thread libcpu
    'rt-thread/libcpu/arm/common/backtrace.c',
    'rt-thread/libcpu/arm/common/div0.c',
    'rt-thread/libcpu/arm/common/showmem.c',
    'rt-thread/libcpu/arm/cortex-m7/cpu_cache.c',
    'rt-thread/libcpu/arm/cortex-m7/cpuport.c',
    # FinSH shell
    'rt-thread/components/finsh/cmd.c',
    'rt-thread/components/finsh/msh.c',
    'rt-thread/components/finsh/shell.c',
    # CMSIS system init
    'libraries/CMSIS/Device/ST/STM32H7xx/Source/Templates/system_stm32h7xx.c',
]

SRC_S = [
    'libraries/CMSIS/Device/ST/STM32H7xx/Source/Templates/gcc/startup_stm32h750xx.S',
    'rt-thread/libcpu/arm/cortex-m7/context_gcc.S',
]

import glob as _glob
SRC_C.extend(sorted(_glob.glob('libraries/STM32H7xx_HAL_Driver/Src/*.c')))

objs = [env.Object(s) for s in SRC_C + SRC_S]
program = env.Program(TARGET, objs)
env.CompilationDatabase('compile_commands.json')

# Generate .bin from .elf after build
env.AddPostAction(program, 'arm-none-eabi-objcopy -O binary $TARGET Debug/rt-thread.bin')
