import os
import sys
import rtconfig

RTT_ROOT = os.path.normpath(os.getcwd() + '/rt-thread')
Export('RTT_ROOT')
Export('rtconfig')

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

env.AppendUnique(CPPDEFINES = [
    'STM32H750xx',
    'SOC_FAMILY_STM32',
    'SOC_SERIES_STM32H7',
    'USE_HAL_DRIVER',
])

# Compilation database (for clangd / IDE navigation) — must be before PrepareBuilding
# so the StaticObject emitter captures all compilation commands.
env.Tool('compilation_db')
env['COMPILATIONDB_USE_ABSPATH'] = True
env.CompilationDatabase('compile_commands.json')

# ========== Collect sources via SConscript chain (no hardcoded file lists) ==========
# PrepareBuilding parses rtconfig.h, collects objects from:
#   - BSP:  root SConscript walks applications/, drivers/, libraries/, packages/
#   - RT-Thread kernel:  rt-thread/src/
#   - libcpu:            rt-thread/libcpu/arm/cortex-m7/ (via ARCH/CPU in rtconfig.py)
#   - components:        rt-thread/components/ (FinSH, etc.)
objs = PrepareBuilding(env, RTT_ROOT, has_libcpu=False)

# Build ELF
program = env.Program(TARGET, objs)

# Generate .bin after build
env.AddPostAction(program, 'arm-none-eabi-objcopy -O binary $TARGET Debug/rt-thread.bin')
