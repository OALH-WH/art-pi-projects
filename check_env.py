#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
============================================================================
check_env.py -- ART-Pi (STM32H750) dev environment checker

Detects:
  1. scons                  -- build system
  2. Python                 -- scripting runtime
  3. arm-none-eabi-gcc      -- ARM GCC cross-compiler (and objcopy/size/gdb)
  4. STM32_Programmer_CLI   -- STM32 flasher
  5. ST-LINK_gdbserver      -- ST-LINK debug server
  6. menuconfig             -- RT-Thread Kconfig tool
  7. openocd                -- OpenOCD debug server (for cortex-debug)

Usage:
  python check_env.py              normal (use cache if valid)
  python check_env.py --force      force full re-detection
  python check_env.py -f           same as --force
  python check_env.py --help       show this help

Outputs:
  setenv.bat          -- Windows env setup script (call setenv.bat)
  setenv.sh           -- Linux / WSL env setup script (source setenv.sh)
  check_env_cache.json -- internal cache for fast re-runs
============================================================================
"""

import os
import sys
import json
import shutil
import argparse
from pathlib import Path
from datetime import datetime


class EnvChecker:
    """ART-Pi development environment checker"""
    
    def __init__(self):
        # Configuration
        self.project_dir = Path(__file__).parent.absolute()
        self.cache_file = self.project_dir / "check_env_cache.json"
        self.scons_env = self.project_dir / "scons.env"
        
        # Default paths
        self.default_python = Path(r"C:\Python314\python.exe")
        self.default_toolchain = Path.home() / "MyFiles/IDE/RT-ThreadStudio/platform/env_released/env/tools/gnu_gcc/arm_gcc/mingw/bin"
        self.default_stlink_base = Path.home() / "MyFiles/IDE/RT-ThreadStudio/repo/Extract/Debugger_Support_Packages/STMicroelectronics/ST-LINK_Debugger"
        self.default_env_scripts = Path.home() / "MyFiles/IDE/RT-ThreadStudio/platform/env_released/env/tools/scripts"
        self.default_openocd_base = Path.home() / "MyFiles/IDE/RT-ThreadStudio/repo/Extract/Debugger_Support_Packages"
        
        # Detection results
        self.results = {
            'scons': {'found': False, 'path': None},
            'python': {'found': False, 'path': None},
            'gcc': {'found': False, 'rtt_exec_path': None},
            'objcopy': {'found': False},
            'size': {'found': False},
            'gdb': {'found': False},
            'stm32_programmer': {'found': False, 'path': None},
            'stlink_gdb': {'found': False, 'path': None},
            'menuconfig': {'found': False, 'path': None, 'env_tools_dir': None},
            'openocd': {'found': False, 'path': None}
        }
        
    def run(self, force=False, quiet=False):
        """Main execution method"""
        if not quiet:
            print()
            print("=" * 60)
            print("  ART-Pi (STM32H750) Dev Environment Checker")
            print("=" * 60)
            print()
        
        # Try loading from cache
        if not force:
            if self._try_load_cache():
                if not quiet:
                    print("[CACHE] Using cached configuration.")
                    print("[CACHE] Run 'python check_env.py --force' to re-detect all.")
                self._print_summary()
                self._generate_env_scripts()
                return True
        
        # Full detection
        if not quiet:
            print("[SCAN] Scanning for development tools...\n")
        
        self._detect_scons()
        self._detect_python()
        self._detect_arm_gcc()
        self._detect_stm32_programmer()
        self._detect_stlink_gdbserver()
        self._detect_menuconfig()
        self._detect_openocd()
        
        # Save cache and generate scripts
        self._save_cache()
        self._generate_env_scripts()
        
        if not quiet:
            self._print_summary()
            
        return True
    
    def _try_load_cache(self):
        """Try to load and validate cache"""
        if not self.cache_file.exists():
            print("[CACHE] No cache file found (first run).")
            return False
        
        try:
            print("[CACHE] Loading cache...")
            with open(self.cache_file, 'r') as f:
                cache = json.load(f)
        except Exception:
            print("[CACHE] Cache invalid, re-detecting...")
            return False
        
        # Validate cached paths
        if not self._validate_cache(cache):
            print("[CACHE] Cache invalid, re-detecting...")
            return False
        
        # Restore results from cache
        for key in self.results:
            if key in cache:
                self.results[key] = cache[key]
        
        return True
    
    def _validate_cache(self, cache):
        """Validate cached paths exist"""
        try:
            if cache.get('scons', {}).get('found') and not os.path.exists(cache['scons']['path']):
                return False
            if cache.get('python', {}).get('found') and not os.path.exists(cache['python']['path']):
                return False
            if cache.get('gcc', {}).get('found'):
                gcc_path = Path(cache['gcc']['rtt_exec_path']) / "arm-none-eabi-gcc.exe"
                if not gcc_path.exists():
                    return False
            if cache.get('stm32_programmer', {}).get('found'):
                if not os.path.exists(cache['stm32_programmer']['path']):
                    return False
            if cache.get('stlink_gdb', {}).get('found'):
                if not os.path.exists(cache['stlink_gdb']['path']):
                    return False
            if cache.get('menuconfig', {}).get('found'):
                if not os.path.exists(cache['menuconfig']['path']):
                    return False
            if cache.get('openocd', {}).get('found'):
                if not os.path.exists(cache['openocd']['path']):
                    return False
        except Exception:
            return False
        return True
    
    def _save_cache(self):
        """Save detection results to cache"""
        print(f"\n[CACHE] Saving detection results to {self.cache_file}")
        
        cache = self.results.copy()
        cache['generated'] = datetime.now().isoformat()
        cache['project_dir'] = str(self.project_dir)
        
        with open(self.cache_file, 'w') as f:
            json.dump(cache, f, indent=2)
    
    def _detect_scons(self):
        """Detect scons build system"""
        scons_path = shutil.which('scons')
        if scons_path:
            self.results['scons'] = {'found': True, 'path': scons_path}
            print(f"  [ OK ] scons       : {scons_path}")
        else:
            print(f"  [MISS] scons       : not found (pip install scons)")
    
    def _detect_python(self):
        """Detect Python interpreter"""
        # Check default path first
        if self.default_python.exists():
            self.results['python'] = {'found': True, 'path': str(self.default_python)}
            print(f"  [ OK ] Python      : {self.default_python}")
            return
        
        # Check PATH
        python_path = shutil.which('python') or shutil.which('python3')
        if python_path:
            self.results['python'] = {'found': True, 'path': python_path}
            print(f"  [ OK ] Python      : {python_path} (from PATH)")
        else:
            print(f"  [MISS] Python      : not found (expected: {self.default_python})")
    
    def _detect_arm_gcc(self):
        """Detect ARM GCC toolchain"""
        gcc_exe = "arm-none-eabi-gcc.exe"
        
        # Check cached/given RTT_EXEC_PATH first
        rtt_path = self.results['gcc'].get('rtt_exec_path')
        if rtt_path:
            gcc_path = Path(rtt_path) / gcc_exe
            if gcc_path.exists():
                self.results['gcc']['found'] = True
                print(f"  [ OK ] arm-none-eabi-gcc : {gcc_path}")
                self._check_gcc_tools()
                return
        
        # Check default toolchain path
        gcc_path = self.default_toolchain / gcc_exe
        if gcc_path.exists():
            self.results['gcc'] = {
                'found': True,
                'rtt_exec_path': str(self.default_toolchain)
            }
            print(f"  [ OK ] arm-none-eabi-gcc : {gcc_path}")
            self._check_gcc_tools()
            return
        
        # Check PATH
        gcc_path = shutil.which('arm-none-eabi-gcc')
        if gcc_path:
            rtt_exec_path = str(Path(gcc_path).parent)
            self.results['gcc'] = {
                'found': True,
                'rtt_exec_path': rtt_exec_path
            }
            print(f"  [ OK ] arm-none-eabi-gcc : {gcc_path} (from PATH)")
            self._check_gcc_tools()
            return
        
        print("  [MISS] arm-none-eabi-gcc : not found")
    
    def _check_gcc_tools(self):
        """Check for additional GCC tools"""
        rtt_path = Path(self.results['gcc']['rtt_exec_path'])
        
        for tool in ['arm-none-eabi-objcopy.exe', 'arm-none-eabi-size.exe', 'arm-none-eabi-gdb.exe']:
            key = tool.replace('arm-none-eabi-', '').replace('.exe', '')
            if (rtt_path / tool).exists():
                self.results[key]['found'] = True
                print(f"  [ OK ] {tool}")
            else:
                print(f"  [MISS] {tool}")
    
    def _detect_stm32_programmer(self):
        """Detect STM32_Programmer_CLI"""
        if not self.default_stlink_base.exists():
            print(f"  [MISS] STM32_Programmer_CLI : ST-LINK Debugger not installed")
            print(f"         Expected: {self.default_stlink_base}")
            return
        
        # Search for STM32_Programmer_CLI in subdirectories
        for stlink_dir in self.default_stlink_base.iterdir():
            if stlink_dir.is_dir():
                prog_path = stlink_dir / "tools" / "bin" / "STM32_Programmer_CLI.exe"
                if prog_path.exists():
                    self.results['stm32_programmer'] = {
                        'found': True,
                        'path': str(prog_path)
                    }
                    print(f"  [ OK ] STM32_Programmer_CLI : {prog_path}")
                    return
        
        print(f"  [MISS] STM32_Programmer_CLI : not found under {self.default_stlink_base}")
    
    def _detect_stlink_gdbserver(self):
        """Detect ST-LINK_gdbserver"""
        prog_path = self.results['stm32_programmer'].get('path')
        if prog_path:
            gdb_path = Path(prog_path.replace('STM32_Programmer_CLI.exe', 'ST-LINK_gdbserver.exe'))
            if gdb_path.exists():
                self.results['stlink_gdb'] = {
                    'found': True,
                    'path': str(gdb_path)
                }
                print(f"  [ OK ] ST-LINK_gdbserver : {gdb_path}")
            else:
                print("  [MISS] ST-LINK_gdbserver : not found alongside STM32_Programmer_CLI")
        else:
            print("  [MISS] ST-LINK_gdbserver : skipped (no STM32_Programmer_CLI)")
    
    def _detect_menuconfig(self):
        """Detect menuconfig (RT-Thread Kconfig)"""
        menuconfig_py = self.default_env_scripts / "menuconfig.py"
        if menuconfig_py.exists():
            self.results['menuconfig'] = {
                'found': True,
                'path': str(menuconfig_py),
                'env_tools_dir': str(self.default_env_scripts)
            }
            print(f"  [ OK ] menuconfig    : {menuconfig_py}")
        else:
            print(f"  [MISS] menuconfig    : not found")
            print(f"         Expected: {menuconfig_py}")
    
    def _detect_openocd(self):
        """Detect OpenOCD debug server"""
        # Check default RT-Thread Studio bundled OpenOCD first
        if self.default_openocd_base.exists():
            for ocp_dir in sorted(self.default_openocd_base.iterdir(), reverse=True):
                if ocp_dir.is_dir() and "OpenOCD" in ocp_dir.name:
                    ocp_bin = ocp_dir / "bin" / "openocd.exe"
                    if ocp_bin.exists():
                        self.results['openocd'] = {
                            'found': True,
                            'path': str(ocp_bin)
                        }
                        print(f"  [ OK ] openocd      : {ocp_bin}")
                        return

        # Check PATH
        ocp_path = shutil.which('openocd')
        if ocp_path:
            self.results['openocd'] = {'found': True, 'path': ocp_path}
            print(f"  [ OK ] openocd      : {ocp_path} (from PATH)")
            return

        print(f"  [MISS] openocd      : not found")
        print(f"         Expected under: {self.default_openocd_base}")

    def _print_summary(self):
        """Print detection results summary"""
        print()
        print("=" * 60)
        print("  Detection Results")
        print("=" * 60)
        
        items = [
            ("scons", self.results['scons']),
            ("Python", self.results['python']),
            ("arm-none-eabi-gcc", {
                'found': self.results['gcc']['found'],
                'path': f"{self.results['gcc'].get('rtt_exec_path', '')}\\arm-none-eabi-gcc.exe" if self.results['gcc'].get('rtt_exec_path') else None
            }),
            ("arm-none-eabi-objcopy", {
                'found': self.results['objcopy']['found'],
                'path': f"{self.results['gcc'].get('rtt_exec_path', '')}\\arm-none-eabi-objcopy.exe" if self.results['gcc'].get('rtt_exec_path') else None
            }),
            ("arm-none-eabi-size", {
                'found': self.results['size']['found'],
                'path': f"{self.results['gcc'].get('rtt_exec_path', '')}\\arm-none-eabi-size.exe" if self.results['gcc'].get('rtt_exec_path') else None
            }),
            ("arm-none-eabi-gdb", {
                'found': self.results['gdb']['found'],
                'path': f"{self.results['gcc'].get('rtt_exec_path', '')}\\arm-none-eabi-gdb.exe" if self.results['gcc'].get('rtt_exec_path') else None
            }),
            ("STM32_Programmer_CLI", self.results['stm32_programmer']),
            ("ST-LINK_gdbserver", self.results['stlink_gdb']),
            ("menuconfig", self.results['menuconfig']),
            ("openocd", self.results['openocd'])
        ]
        
        for name, info in items:
            self._print_one(name, info.get('found', False), info.get('path'))
        
        print()
        print("[NOTE] Environment generated:")
        print(f"       Platform:   source {self.scons_env}")
        print()
    
    @staticmethod
    def _print_one(name, found, path=None):
        """Print one detection result"""
        if found:
            print(f"  [ OK ] {name}")
            if path:
                print(f"         {path}")
        else:
            print(f"  [MISS] {name}")
    
    def _generate_env_scripts(self):
        self._generate_scons_env()
        self._generate_vscode_settings()

    def _generate_vscode_settings(self):
        """Generate .vscode/settings.json with detected tool paths for cortex-debug"""
        settings_dir = self.project_dir / ".vscode"
        settings_dir.mkdir(exist_ok=True)
        settings_file = settings_dir / "settings.json"

        # Read existing settings to preserve manual entries
        existing = {}
        if settings_file.exists():
            try:
                with open(settings_file, 'r') as f:
                    existing = json.load(f)
            except Exception:
                pass

        # Update with detected paths
        openocd_path = self.results['openocd'].get('path', '')
        rtt_exec_path = self.results['gcc'].get('rtt_exec_path', '')

        if openocd_path:
            existing['cortex-debug.openocdPath'] = openocd_path.replace(chr(92), '/')
        if rtt_exec_path:
            existing['cortex-debug.armToolchainPath'] = rtt_exec_path.replace(chr(92), '/')

        with open(settings_file, 'w', encoding='utf-8') as f:
            json.dump(existing, f, indent=4)

        print(f"[GEN] {settings_file}")
    
    def _generate_scons_env(self):
        """Generate dotenv file for SCons (KEY=VALUE pairs only, no shell syntax)"""
        print(f"[GEN] {self.scons_env}")

        rtt_exec_path = self.results['gcc'].get('rtt_exec_path', '')
        stm32_prog_path = self.results['stm32_programmer'].get('path', '')
        stlink_gdb_path = self.results['stlink_gdb'].get('path', '')
        menuconfig_path = self.results['menuconfig'].get('path', '')
        env_tools_dir = self.results['menuconfig'].get('env_tools_dir', '')
        python_path = self.results['python'].get('path', '')
        openocd_path = self.results['openocd'].get('path', '')

        lines = []

        # Use forward slashes to prevent python-dotenv from interpreting
        # backslash sequences (\a, \b, \n, etc.) as escape characters
        fs = lambda p: p.replace(chr(92), '/') if p else ''
        if python_path:
            lines.append(f'PYTHON_PATH="{fs(python_path)}"')
        if rtt_exec_path:
            lines.append(f'RTT_EXEC_PATH="{fs(rtt_exec_path)}"')
        if stm32_prog_path:
            lines.append(f'STM32_PROG_PATH="{fs(stm32_prog_path)}"')
        if stlink_gdb_path:
            lines.append(f'STLINK_GDB_SERVER="{fs(stlink_gdb_path)}"')
        if menuconfig_path:
            lines.append(f'MENUCONFIG_PATH="{fs(menuconfig_path)}"')
        if env_tools_dir:
            lines.append(f'ENV_TOOLS_DIR="{fs(env_tools_dir)}"')
        if openocd_path:
            lines.append(f'OPENOCD_PATH="{fs(openocd_path)}"')
        lines.append(f'RTT_CC="gcc"')

        # Write file with Unix line endings
        content = '\n'.join(lines) + '\n'
        with open(self.scons_env, 'w', encoding='utf-8', newline='\n') as f:
            f.write(content)

        # Make executable on Unix-like systems
        try:
            os.chmod(self.scons_env, 0o755)
        except OSError:
            pass  # Ignore on Windows


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='ART-Pi (STM32H750) dev environment checker',
        add_help=False
    )
    parser.add_argument('--force', '-f', action='store_true',
                       help='Force full re-detection (skip cache)')
    parser.add_argument('--help', '-h', action='store_true',
                       help='Show this help message')
    parser.add_argument('--quiet', '-q', action='store_true',
                       help='Quiet mode (minimal output)')
    
    args = parser.parse_args()
    
    if args.help:
        parser.print_help()
        print("\nDetected tools:")
        print("  1. scons                   -- SCons build system")
        print("  2. Python 3.x              -- Scripting runtime")
        print("  3. arm-none-eabi-gcc       -- ARM GCC cross-compiler")
        print("  4. arm-none-eabi-objcopy   -- Binary conversion")
        print("  5. arm-none-eabi-size      -- ELF size analyzer")
        print("  6. arm-none-eabi-gdb       -- GDB debugger")
        print("  7. STM32_Programmer_CLI    -- STM32 flasher (ST-Link)")
        print("  8. ST-LINK_gdbserver       -- ST-LINK GDB server")
        print("  9. menuconfig              -- RT-Thread Kconfig configurator")
        print(" 10. openocd                 -- OpenOCD debug server")
        print("\nCache:")
        print("  - First run saves paths to check_env_cache.json")
        print("  - Second run reads cache and only does quick path validation")
        print("  - If cached paths are invalid, falls back to full detection")
        print("  - Use --force to skip cache entirely")
        sys.exit(0)
    
    checker = EnvChecker()
    success = checker.run(force=args.force, quiet=args.quiet)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()