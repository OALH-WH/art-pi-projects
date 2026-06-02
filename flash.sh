#!/bin/bash
# ============================================================================
# flash.sh - 构建并烧录 ART-Pi 应用程序到 QSPI Flash (0x90000000)
#
# 用法:
#   ./flash.sh         构建 + 烧录
#   ./flash.sh --skip  仅烧录（跳过构建）
#   ./flash.sh --help  显示帮助
#
# 烧录工具: STM32_Programmer_CLI (ST-Link)
# 外部 Flash: W25Q64 (QSPI, 映射地址 0x90000000)
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# STM32_Programmer_CLI 路径
STM32_PROG="c:/Users/OALH/MyFiles/IDE/RT-ThreadStudio/repo/Extract/Debugger_Support_Packages/STMicroelectronics/ST-LINK_Debugger/2.11.0/tools/bin/STM32_Programmer_CLI.exe"

# 外部 Flash 加载器（用于 W25Q64）
STLDR="$SCRIPT_DIR/board/stldr/ART-Pi_W25Q64.stldr"

# 目标文件
BIN="rtthread.bin"
ELDF="rtthread.elf"
FLASH_ADDR="0x90000000"

# ============================================================================
# 帮助
# ============================================================================
if [ "$1" = "--help" ]; then
    sed -n '2,12p' "$0"
    exit 0
fi

# ============================================================================
# 1. 构建
# ============================================================================
if [ "$1" != "--skip" ]; then
    echo "=========================================="
    echo "  Step 1: 编译项目 (scons)"
    echo "=========================================="

    if ! command -v scons &>/dev/null; then
        echo "错误: 未找到 scons，请确保 scons 已安装并在 PATH 中"
        echo "  pip install scons"
        exit 1
    fi

    scons -j4

    if [ ! -f "$BIN" ]; then
        echo "错误: 编译完成但未找到 $BIN"
        echo "请检查 POST_ACTION 是否生成了 .bin 文件"
        exit 1
    fi

    echo "构建成功: $BIN"
else
    echo "跳过构建步骤（--skip）"
fi

# ============================================================================
# 2. 烧录到 QSPI Flash
# ============================================================================
echo ""
echo "=========================================="
echo "  Step 2: 烧录到 QSPI Flash @ $FLASH_ADDR"
echo "=========================================="

if [ ! -f "$STM32_PROG" ]; then
    echo "错误: 未找到 STM32_Programmer_CLI"
    echo "  请检查路径: $STM32_PROG"
    echo "  或在 RT-Thread Studio 中安装 ST-LINK Debugger 包"
    exit 1
fi

if [ ! -f "$STLDR" ]; then
    echo "错误: 未找到外部 Flash 加载器"
    echo "  请检查路径: $STLDR"
    exit 1
fi

if [ ! -f "$BIN" ]; then
    echo "错误: 未找到二进制文件 $BIN，请先构建"
    exit 1
fi

echo "烧录文件: $BIN ($(du -h "$BIN" | cut -f1))"
echo "目标地址: $FLASH_ADDR"
echo ""

# 执行烧录
"$STM32_PROG" -c port=SWD mode=UR \
    -el "$STLDR" \
    -w "$BIN" "$FLASH_ADDR" \
    -v \
    -rst

echo ""
echo "=========================================="
echo "  烧录完成！"
echo "=========================================="
echo "提示: 如果板上有 bootloader，复位后会自动跳转到此应用"
echo "      短按 RESET 键即可启动"
