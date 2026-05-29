import os
import shutil

# toolchains options
ARCH = 'arm'
CPU = 'cortex-m7'
CROSS_TOOL = 'gcc'

# cross_tool provides the cross compiler
# EXEC_PATH is the compiler execute path, for example, CodeSourcery, Keil MDK, IAR
PLATFORM = 'gcc'
EXEC_PATH = ''

if os.getenv('RTT_EXEC_PATH'):
    EXEC_PATH = os.getenv('RTT_EXEC_PATH')

# Try to find toolchain in PATH (e.g. added by RT-Thread Studio or user env)
if not EXEC_PATH:
    gcc_path = shutil.which('arm-none-eabi-gcc')
    if gcc_path:
        EXEC_PATH = os.path.dirname(gcc_path)

PREFIX = 'arm-none-eabi-'
CC = PREFIX + 'gcc'
AS = PREFIX + 'gcc'
AR = PREFIX + 'ar'
CXX = PREFIX + 'g++'
LINK = PREFIX + 'gcc'
TARGET_EXT = 'elf'
SIZE = PREFIX + 'size'
OBJDUMP = PREFIX + 'objdump'
OBJCPY = PREFIX + 'objcopy'
DEVICE = ' -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16'
CFLAGS = DEVICE + ' -O0 -ffunction-sections -fdata-sections -Wall -g -gdwarf-2 -std=gnu11'
AFLAGS = DEVICE + ' -x assembler-with-cpp -c -g -gdwarf-2 -Xassembler -mimplicit-it=thumb'
LFLAGS = DEVICE + ' -O0 -ffunction-sections -fdata-sections -Wl,-Map,Debug/output.map -Wl,--gc-sections -T linkscripts/STM32H750XBHx/link.lds -nostartfiles --specs=nano.specs --specs=nosys.specs'
CPATH = ''
LPATH = ''
CXXFLAGS = DEVICE + ' -O0 -ffunction-sections -fdata-sections -Wall -g -gdwarf-2'
POST_ACTION = ''
