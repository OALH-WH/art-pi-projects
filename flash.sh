OPENOCD="/c/Users/OALH/MyFiles/IDE/RT-ThreadStudio/repo/Extract/Debugger_Support_Packages/OpenOCD-20250710-0.12.0/bin/openocd"
SCRIPT_DIR="/c/Users/OALH/MyFiles/IDE/RT-ThreadStudio/repo/Extract/Debugger_Support_Packages/OpenOCD-20250710-0.12.0/share/openocd/scripts"
ELF="Debug/rt-thread.elf"
BIN="Debug/rt-thread.bin"
BSP_DIR="c:/Users/OALH/MyFiles/IDE/RT-ThreadStudio/workspace/art-pi-projects"

echo "==> Converting ELF to binary ..."
arm-none-eabi-objcopy -O binary "$ELF" "$BIN"

echo "==> Programming bootloader to internal flash @ 0x08000000 ..."
"$OPENOCD" -s "$SCRIPT_DIR" \
    -f interface/stlink.cfg \
    -f target/stm32h750xb.cfg \
    -c "init" \
    -c "reset halt" \
    -c "flash write_image erase $BSP_DIR/$BIN 0x08000000" \
    -c "reset run" \
    -c "shutdown"
