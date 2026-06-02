@echo off
REM ============================================================================
REM check_env.bat -- ART-Pi (STM32H750) dev environment checker
REM
REM Detects:
REM   1. scons                  -- build system
REM   2. Python                 -- scripting runtime
REM   3. arm-none-eabi-gcc      -- ARM GCC cross-compiler (and objcopy/size/gdb)
REM   4. STM32_Programmer_CLI   -- STM32 flasher
REM   5. ST-LINK_gdbserver      -- ST-LINK debug server
REM   6. menuconfig             -- RT-Thread Kconfig tool
REM
REM Usage:
REM   check_env.bat              normal (use cache if valid)
REM   check_env.bat --force      force full re-detection
REM   check_env.bat -f           same as --force
REM   check_env.bat --help       show this help
REM
REM Outputs:
REM   setenv.bat          -- Windows env setup script (call setenv.bat)
REM   setenv.sh           -- Linux / WSL env setup script (source setenv.sh)
REM   check_env_cache.bat  -- internal cache for fast re-runs
REM ============================================================================

setlocal enabledelayedexpansion

REM ============================
REM  0. Configuration
REM ============================
set "PROJECT_DIR=%~dp0"
set "CACHE_FILE=%PROJECT_DIR%check_env_cache.bat"
set "SETENV_BAT=%PROJECT_DIR%setenv.bat"
set "SETENV_SH=%PROJECT_DIR%setenv.sh"

REM default paths (from SConstruct, flash.sh, CLAUDE.md of this project)
set "DEFAULT_PYTHON=C:\Python314\python.exe"
set "DEFAULT_TOOLCHAIN=%USERPROFILE%\MyFiles\IDE\RT-ThreadStudio\platform\env_released\env\tools\gnu_gcc\arm_gcc\mingw\bin"
set "DEFAULT_STLINK_BASE=%USERPROFILE%\MyFiles\IDE\RT-ThreadStudio\repo\Extract\Debugger_Support_Packages\STMicroelectronics\ST-LINK_Debugger"
set "DEFAULT_ENV_SCRIPTS=%USERPROFILE%\MyFiles\IDE\RT-ThreadStudio\platform\env_released\env\tools\scripts"

REM result flags (0/1) and paths
set "SCONS_FOUND=0"       & set "SCONS_PATH="
set "PYTHON_FOUND=0"      & set "PYTHON_PATH="
set "GCC_FOUND=0"         & set "RTT_EXEC_PATH="
set "OBJCPY_FOUND=0"
set "SIZE_FOUND=0"
set "GDB_FOUND=0"
set "STM32_PROG_FOUND=0"  & set "STM32_PROG_PATH="
set "STLINK_GDB_FOUND=0"  & set "STLINK_GDB_PATH="
set "MENUCONFIG_FOUND=0"  & set "MENUCONFIG_PATH=" & set "ENV_TOOLS_DIR="

REM ============================
REM  1. Parse args
REM ============================
set "FORCE=0"
if /I "%~1"=="--force"  set "FORCE=1"
if /I "%~1"=="-f"       set "FORCE=1"
if /I "%~1"=="--help"   goto :help
if /I "%~1"=="-h"       goto :help

echo.
echo ============================================================
echo   ART-Pi (STM32H750) Dev Environment Checker
echo ============================================================
echo.

REM ============================
REM  2. Try loading from cache
REM ============================
if "%FORCE%"=="0" (
    call :try_load_cache
    if "!CACHE_LOADED!"=="1" (
        echo [CACHE] Using cached configuration.
        echo [CACHE] Run "check_env.bat --force" to re-detect all.
        echo.
        goto :print_summary
    )
)

REM ============================
REM  3. Full detection
REM ============================
echo [SCAN] Scanning for development tools...
echo.

call :detect_scons
call :detect_python
call :detect_arm_gcc
call :detect_stm32_programmer
call :detect_stlink_gdbserver
call :detect_menuconfig

REM ============================
REM  4. Save cache + generate setenv scripts
REM ============================
call :save_cache
call :generate_setenv_bat
call :generate_setenv_sh

REM ============================
REM  5. Summary
REM ============================
:print_summary
echo.
echo ============================================================
echo   Detection Results
echo ============================================================

call :print_one "scons"                    !SCONS_FOUND!        "!SCONS_PATH!"
call :print_one "Python"                   !PYTHON_FOUND!       "!PYTHON_PATH!"
call :print_one "arm-none-eabi-gcc"        !GCC_FOUND!          "!RTT_EXEC_PATH!\arm-none-eabi-gcc.exe"
call :print_one "arm-none-eabi-objcopy"    !OBJCPY_FOUND!       "!RTT_EXEC_PATH!\arm-none-eabi-objcopy.exe"
call :print_one "arm-none-eabi-size"       !SIZE_FOUND!         "!RTT_EXEC_PATH!\arm-none-eabi-size.exe"
call :print_one "arm-none-eabi-gdb"        !GDB_FOUND!          "!RTT_EXEC_PATH!\arm-none-eabi-gdb.exe"
call :print_one "STM32_Programmer_CLI"     !STM32_PROG_FOUND!   "!STM32_PROG_PATH!"
call :print_one "ST-LINK_gdbserver"        !STLINK_GDB_FOUND!   "!STLINK_GDB_PATH!"
call :print_one "menuconfig"              !MENUCONFIG_FOUND!   "!MENUCONFIG_PATH!"

echo.
if "%CACHE_LOADED%"=="1" (
    echo [NOTE] Results from cache. Use --force to re-detect.
) else (
    echo [NOTE] Environment scripts generated:
    echo        Windows: call setenv.bat
    echo        Linux:   source setenv.sh
    echo.
)
echo.

if "%FORCE%"=="1" endlocal
goto :eof


REM ==================================================================
REM  Help
REM ==================================================================
:help
echo Usage: check_env.bat [OPTION]
echo.
echo Options:
echo   --force, -f   Force full re-detection (skip cache)
echo   --help, -h    Show this help
echo.
echo Detected tools:
echo   1. scons                   -- SCons build system
echo   2. Python 3.x              -- Scripting runtime
echo   3. arm-none-eabi-gcc       -- ARM GCC cross-compiler
echo   4. arm-none-eabi-objcopy   -- Binary conversion
echo   5. arm-none-eabi-size      -- ELF size analyzer
echo   6. arm-none-eabi-gdb       -- GDB debugger
echo   7. STM32_Programmer_CLI    -- STM32 flasher (ST-Link)
echo   8. ST-LINK_gdbserver       -- ST-LINK GDB server
echo   9. menuconfig              -- RT-Thread Kconfig configurator
echo.
echo Cache:
echo   - First run saves paths to check_env_cache.bat
echo   - Second run reads cache and only does quick path validation
echo   - If cached paths are invalid, falls back to full detection
echo   - Use --force to skip cache entirely
echo.
goto :eof


REM ==================================================================
REM  Print one result
REM ==================================================================
:print_one
set "_NAME=%~1"
set "_FOUND=%~2"
set "_PATH=%~3"
if "%_FOUND%"=="1" (
    echo   [ OK ] %_NAME%
    if not "%_PATH%"=="" echo          %_PATH%
) else (
    echo   [MISS] %_NAME%
)
goto :eof


REM ==================================================================
REM  Load from cache
REM ==================================================================
:try_load_cache
set "CACHE_LOADED=0"
if not exist "%CACHE_FILE%" goto :cache_miss

REM cache exists - parse it
echo [CACHE] Loading cache...
REM cache exists - load it via call (handles CRLF natively)
echo [CACHE] Loading cache...
call "%CACHE_FILE%"

REM reconstruct FOUND flags from loaded variables
if defined SCONS_PATH        set "SCONS_FOUND=1"
if defined PYTHON_PATH       set "PYTHON_FOUND=1"
if defined RTT_EXEC_PATH     set "GCC_FOUND=1"
if defined STM32_PROG_PATH   set "STM32_PROG_FOUND=1"
if defined STLINK_GDB_PATH   set "STLINK_GDB_FOUND=1"
if defined MENUCONFIG_PATH   set "MENUCONFIG_FOUND=1"
REM these are flags in the cache, not path variables
if defined OBJCPY_EXISTS     set "OBJCPY_FOUND=!OBJCPY_EXISTS!"
if defined SIZE_EXISTS       set "SIZE_FOUND=!SIZE_EXISTS!"
if defined GDB_EXISTS        set "GDB_FOUND=!GDB_EXISTS!"

REM quick path validation - skip to re-detect if paths are stale
set "CACHE_OK=1"
if not defined SCONS_PATH           set "CACHE_OK=0"
if defined SCONS_PATH if not exist "!SCONS_PATH!" set "CACHE_OK=0"
if not defined PYTHON_PATH          set "CACHE_OK=0"
if defined PYTHON_PATH if not exist "!PYTHON_PATH!" set "CACHE_OK=0"
if not defined RTT_EXEC_PATH        set "CACHE_OK=0"
if defined RTT_EXEC_PATH if not exist "!RTT_EXEC_PATH!\arm-none-eabi-gcc.exe" set "CACHE_OK=0"
if defined STM32_PROG_PATH if not exist "!STM32_PROG_PATH!" set "CACHE_OK=0"
if defined STLINK_GDB_PATH if not exist "!STLINK_GDB_PATH!" set "CACHE_OK=0"
if defined MENUCONFIG_PATH if not exist "!MENUCONFIG_PATH!" set "CACHE_OK=0"

if "!CACHE_OK!"=="0" (
    echo [CACHE] Cache invalid, re-detecting...
    set "CACHE_LOADED=0"
    goto :eof
)
set "CACHE_LOADED=1"
goto :eof

:cache_miss
echo [CACHE] No cache file found (first run).
set "CACHE_LOADED=0"
goto :eof

REM ==================================================================
REM  Save cache
REM ==================================================================
:save_cache
echo.
echo [CACHE] Saving detection results to %CACHE_FILE%

REM -- use redirect-before-echo syntax to avoid trailing spaces --
> "%CACHE_FILE%" echo @echo off
>>"%CACHE_FILE%" echo REM check_env.cache - ART-Pi environment path cache [auto-generated]
>>"%CACHE_FILE%" echo REM Generated: %DATE% %TIME%
>>"%CACHE_FILE%" echo.
>>"%CACHE_FILE%" echo set "SCONS_PATH=!SCONS_PATH!"
>>"%CACHE_FILE%" echo set "PYTHON_PATH=!PYTHON_PATH!"
>>"%CACHE_FILE%" echo set "RTT_EXEC_PATH=!RTT_EXEC_PATH!"
>>"%CACHE_FILE%" echo set "OBJCPY_EXISTS=!OBJCPY_FOUND!"
>>"%CACHE_FILE%" echo set "SIZE_EXISTS=!SIZE_FOUND!"
>>"%CACHE_FILE%" echo set "GDB_EXISTS=!GDB_FOUND!"
>>"%CACHE_FILE%" echo set "STM32_PROG_PATH=!STM32_PROG_PATH!"
if defined STLINK_GDB_PATH (
    >>"%CACHE_FILE%" echo set "STLINK_GDB_PATH=!STLINK_GDB_PATH!"
)
if defined MENUCONFIG_PATH (
    >>"%CACHE_FILE%" echo set "MENUCONFIG_PATH=!MENUCONFIG_PATH!"
    >>"%CACHE_FILE%" echo set "ENV_TOOLS_DIR=!ENV_TOOLS_DIR!"
)

goto :eof


REM ==================================================================
REM  Generate: setenv.bat
REM ==================================================================
:generate_setenv_bat
echo [GEN] setenv.bat

echo @echo off > "%SETENV_BAT%"
echo REM ============================================================ >> "%SETENV_BAT%"
echo REM setenv.bat - ART-Pi dev environment variables [auto-generated] >> "%SETENV_BAT%"
echo REM Usage: call setenv.bat >> "%SETENV_BAT%"
echo REM ============================================================ >> "%SETENV_BAT%"
echo. >> "%SETENV_BAT%"
echo REM --- Tool paths --- >> "%SETENV_BAT%"
if defined SCONS_PATH      echo set "SCONS_PATH=!SCONS_PATH!" >> "%SETENV_BAT%"
if defined PYTHON_PATH     echo set "PYTHON_PATH=!PYTHON_PATH!" >> "%SETENV_BAT%"
if defined RTT_EXEC_PATH   echo set "RTT_EXEC_PATH=!RTT_EXEC_PATH!" >> "%SETENV_BAT%"
if defined STM32_PROG_PATH echo set "STM32_PROG_PATH=!STM32_PROG_PATH!" >> "%SETENV_BAT%"
if defined STLINK_GDB_PATH echo set "STLINK_GDB_SERVER=!STLINK_GDB_PATH!" >> "%SETENV_BAT%"
if defined MENUCONFIG_PATH echo set "MENUCONFIG_PATH=!MENUCONFIG_PATH!" >> "%SETENV_BAT%"
if defined ENV_TOOLS_DIR   echo set "ENV_TOOLS_DIR=!ENV_TOOLS_DIR!" >> "%SETENV_BAT%"
echo. >> "%SETENV_BAT%"
echo REM --- RT-Thread build variables --- >> "%SETENV_BAT%"
echo set "RTT_CC=gcc" >> "%SETENV_BAT%"
if defined RTT_EXEC_PATH   echo set "RTT_EXEC_PATH=!RTT_EXEC_PATH!" >> "%SETENV_BAT%"
echo. >> "%SETENV_BAT%"
echo REM --- PATH (append, avoid duplicates) --- >> "%SETENV_BAT%"
if defined RTT_EXEC_PATH   echo set "PATH=!RTT_EXEC_PATH!;%%PATH%%" >> "%SETENV_BAT%"
if defined ENV_TOOLS_DIR   echo set "PATH=!ENV_TOOLS_DIR!;%%PATH%%" >> "%SETENV_BAT%"
echo. >> "%SETENV_BAT%"
echo REM --- Flash helpers --- >> "%SETENV_BAT%"
echo set "FLASH_ADDR=0x90000000" >> "%SETENV_BAT%"
echo set "BIN_FILE=rtthread.bin" >> "%SETENV_BAT%"
echo set "STLDR=%%~dp0board\stldr\ART-Pi_W25Q64.stldr" >> "%SETENV_BAT%"
echo. >> "%SETENV_BAT%"
echo echo [setenv] ART-Pi environment set >> "%SETENV_BAT%"
if defined RTT_EXEC_PATH   echo echo [setenv]   RTT_EXEC_PATH     = !RTT_EXEC_PATH! >> "%SETENV_BAT%"
if defined STM32_PROG_PATH echo echo [setenv]   STM32_Programmer  = !STM32_PROG_PATH! >> "%SETENV_BAT%"
if defined MENUCONFIG_PATH echo echo [setenv]   menuconfig        = !MENUCONFIG_PATH! >> "%SETENV_BAT%"

goto :eof


REM ==================================================================
REM  Generate: setenv.sh (Linux / WSL)
REM ==================================================================
:generate_setenv_sh
echo [GEN] setenv.sh

REM use disabledelayedexpansion so ! in shebang is preserved literally
setlocal disabledelayedexpansion
echo #!/bin/sh > "%SETENV_SH%"
endlocal
echo # ============================================================ >> "%SETENV_SH%"
echo # setenv.sh - ART-Pi dev environment variables [auto-generated] >> "%SETENV_SH%"
echo # Usage: source setenv.sh >> "%SETENV_SH%"
echo # ============================================================ >> "%SETENV_SH%"
echo. >> "%SETENV_SH%"
echo # --- Tool paths --- >> "%SETENV_SH%"
if defined SCONS_PATH      echo export SCONS_PATH="!SCONS_PATH!" >> "%SETENV_SH%"
if defined PYTHON_PATH     echo export PYTHON_PATH="!PYTHON_PATH!" >> "%SETENV_SH%"
if defined RTT_EXEC_PATH   echo export RTT_EXEC_PATH="!RTT_EXEC_PATH!" >> "%SETENV_SH%"
if defined STM32_PROG_PATH echo export STM32_PROG_PATH="!STM32_PROG_PATH!" >> "%SETENV_SH%"
if defined STLINK_GDB_PATH echo export STLINK_GDB_SERVER="!STLINK_GDB_PATH!" >> "%SETENV_SH%"
if defined MENUCONFIG_PATH echo export MENUCONFIG_PATH="!MENUCONFIG_PATH!" >> "%SETENV_SH%"
if defined ENV_TOOLS_DIR   echo export ENV_TOOLS_DIR="!ENV_TOOLS_DIR!" >> "%SETENV_SH%"
echo. >> "%SETENV_SH%"
echo # --- RT-Thread build variables --- >> "%SETENV_SH%"
echo export RTT_CC="gcc" >> "%SETENV_SH%"
if defined RTT_EXEC_PATH   echo export RTT_EXEC_PATH="!RTT_EXEC_PATH!" >> "%SETENV_SH%"
echo. >> "%SETENV_SH%"
echo # --- PATH --- >> "%SETENV_SH%"
if defined RTT_EXEC_PATH   echo export PATH="${RTT_EXEC_PATH}:$PATH" >> "%SETENV_SH%"
if defined ENV_TOOLS_DIR   echo export PATH="${ENV_TOOLS_DIR}:$PATH" >> "%SETENV_SH%"
echo. >> "%SETENV_SH%"
echo # --- Flash helpers --- >> "%SETENV_SH%"
echo export FLASH_ADDR="0x90000000" >> "%SETENV_SH%"
echo export BIN_FILE="rtthread.bin" >> "%SETENV_SH%"
echo export STLDR="$(dirname "$0")/board/stldr/ART-Pi_W25Q64.stldr" >> "%SETENV_SH%"
echo. >> "%SETENV_SH%"
echo echo "[setenv] ART-Pi environment set" >> "%SETENV_SH%"
if defined RTT_EXEC_PATH   echo echo "[setenv]   RTT_EXEC_PATH     = $RTT_EXEC_PATH" >> "%SETENV_SH%"
if defined STM32_PROG_PATH echo echo "[setenv]   STM32_Programmer  = $STM32_PROG_PATH" >> "%SETENV_SH%"
if defined MENUCONFIG_PATH echo echo "[setenv]   menuconfig        = $MENUCONFIG_PATH" >> "%SETENV_SH%"

goto :eof


REM ==================================================================
REM  Detect: scons
REM ==================================================================
:detect_scons
where scons >nul 2>nul
if %ERRORLEVEL%==0 (
    set "SCONS_FOUND=1"
    for /f "delims=" %%i in ('where scons') do (
        if not defined SCONS_PATH set "SCONS_PATH=%%i"
    )
    echo   [ OK ] scons       : !SCONS_PATH!
) else (
    set "SCONS_FOUND=0"
    echo   [MISS] scons       : not found (pip install scons)
)
goto :eof


REM ==================================================================
REM  Detect: Python
REM ==================================================================
:detect_python
if exist "%DEFAULT_PYTHON%" (
    set "PYTHON_FOUND=1"
    set "PYTHON_PATH=%DEFAULT_PYTHON%"
    echo   [ OK ] Python      : !PYTHON_PATH!
    goto :eof
)

where python >nul 2>nul
if %ERRORLEVEL%==0 (
    set "PYTHON_FOUND=1"
    for /f "delims=" %%i in ('where python') do (
        if not defined PYTHON_PATH set "PYTHON_PATH=%%i"
    )
    echo   [ OK ] Python      : !PYTHON_PATH! (from PATH)
) else (
    set "PYTHON_FOUND=0"
    echo   [MISS] Python      : not found (expected: C:\Python314\python.exe)
)
goto :eof


REM ==================================================================
REM  Detect: ARM GCC toolchain (arm-none-eabi-*)
REM ==================================================================
:detect_arm_gcc
if defined RTT_EXEC_PATH (
    if exist "!RTT_EXEC_PATH!\arm-none-eabi-gcc.exe" (
        set "GCC_FOUND=1"
        echo   [ OK ] arm-none-eabi-gcc : !RTT_EXEC_PATH!\arm-none-eabi-gcc.exe
        goto :check_gcc_tools
    )
)

if exist "%DEFAULT_TOOLCHAIN%\arm-none-eabi-gcc.exe" (
    set "GCC_FOUND=1"
    set "RTT_EXEC_PATH=%DEFAULT_TOOLCHAIN%"
    echo   [ OK ] arm-none-eabi-gcc : !RTT_EXEC_PATH!\arm-none-eabi-gcc.exe
    goto :check_gcc_tools
)

where arm-none-eabi-gcc >nul 2>nul
if %ERRORLEVEL%==0 (
    set "GCC_FOUND=1"
    for /f "delims=" %%i in ('where arm-none-eabi-gcc') do (
        set "GCC_PATH=%%i"
    )
    for %%i in ("!GCC_PATH!") do set "RTT_EXEC_PATH=%%~dpi"
    echo   [ OK ] arm-none-eabi-gcc : !GCC_PATH! (from PATH)
    goto :check_gcc_tools
)

set "GCC_FOUND=0"
echo   [MISS] arm-none-eabi-gcc : not found
goto :eof

:check_gcc_tools
if defined RTT_EXEC_PATH (
    if exist "!RTT_EXEC_PATH!\arm-none-eabi-objcopy.exe" (
        set "OBJCPY_FOUND=1"
        echo   [ OK ] arm-none-eabi-objcopy
    ) else (
        set "OBJCPY_FOUND=0"
        echo   [MISS] arm-none-eabi-objcopy
    )
    if exist "!RTT_EXEC_PATH!\arm-none-eabi-size.exe" (
        set "SIZE_FOUND=1"
        echo   [ OK ] arm-none-eabi-size
    ) else (
        set "SIZE_FOUND=0"
        echo   [MISS] arm-none-eabi-size
    )
    if exist "!RTT_EXEC_PATH!\arm-none-eabi-gdb.exe" (
        set "GDB_FOUND=1"
        echo   [ OK ] arm-none-eabi-gdb
    ) else (
        set "GDB_FOUND=0"
        echo   [MISS] arm-none-eabi-gdb
    )
)
goto :eof


REM ==================================================================
REM  Detect: STM32_Programmer_CLI
REM ==================================================================
:detect_stm32_programmer
if exist "%DEFAULT_STLINK_BASE%" (
    set "STLINK_DIR_FOUND=0"
    for /d %%d in ("%DEFAULT_STLINK_BASE%\*") do (
        if "!STLINK_DIR_FOUND!"=="0" (
            if exist "%%d\tools\bin\STM32_Programmer_CLI.exe" (
                set "STM32_PROG_PATH=%%d\tools\bin\STM32_Programmer_CLI.exe"
                set "STM32_PROG_FOUND=1"
                set "STLINK_DIR_FOUND=1"
                echo   [ OK ] STM32_Programmer_CLI : !STM32_PROG_PATH!
            )
        )
    )
    if "!STM32_PROG_FOUND!"=="0" (
        echo   [MISS] STM32_Programmer_CLI : not found under %DEFAULT_STLINK_BASE%
    )
) else (
    set "STM32_PROG_FOUND=0"
    echo   [MISS] STM32_Programmer_CLI : ST-LINK Debugger not installed
    echo          Expected: %DEFAULT_STLINK_BASE%
)
goto :eof


REM ==================================================================
REM  Detect: ST-LINK_gdbserver
REM ==================================================================
:detect_stlink_gdbserver
set "STLINK_GDB_FOUND=0"
set "STLINK_GDB_PATH="
if defined STM32_PROG_PATH (
    set "GDB_CANDIDATE=!STM32_PROG_PATH:\STM32_Programmer_CLI.exe=\ST-LINK_gdbserver.exe!"
    if exist "!GDB_CANDIDATE!" (
        set "STLINK_GDB_FOUND=1"
        set "STLINK_GDB_PATH=!GDB_CANDIDATE!"
        echo   [ OK ] ST-LINK_gdbserver : !STLINK_GDB_PATH!
    ) else (
        echo   [MISS] ST-LINK_gdbserver : not found alongside STM32_Programmer_CLI
    )
) else (
    echo   [MISS] ST-LINK_gdbserver : skipped (no STM32_Programmer_CLI)
)
goto :eof


REM ==================================================================
REM  Detect: menuconfig (RT-Thread Kconfig)
REM ==================================================================
:detect_menuconfig
if exist "%DEFAULT_ENV_SCRIPTS%\menuconfig.py" (
    set "MENUCONFIG_FOUND=1"
    set "MENUCONFIG_PATH=%DEFAULT_ENV_SCRIPTS%\menuconfig.py"
    set "ENV_TOOLS_DIR=%DEFAULT_ENV_SCRIPTS%"
    echo   [ OK ] menuconfig    : !MENUCONFIG_PATH!
) else (
    set "MENUCONFIG_FOUND=0"
    set "ENV_TOOLS_DIR="
    echo   [MISS] menuconfig    : not found
    echo          Expected: %DEFAULT_ENV_SCRIPTS%\menuconfig.py
)
goto :eof
