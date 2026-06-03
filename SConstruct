import os
import sys
import subprocess
import rtconfig
import traceback
import platform

import os
import sys
from dotenv import load_dotenv
import subprocess

# 加载.env文件中的环境变量
if os.path.exists('scons.env'):
    load_dotenv('scons.env')
else:
    sys.stderr.write("ERROR: scons.env file not found. Please create it with the necessary environment variables.\n")

if os.path.exists('rt-thread'):
    RTT_ROOT = os.path.normpath(os.getcwd() + '/rt-thread')
else:
    RTT_ROOT = os.path.normpath(os.getcwd() + '../../../rt-thread')

sys.path = sys.path + [os.path.join(RTT_ROOT, 'tools')]
try:
    from building import *
except Exception as e:
    print("Error message:", e.message)
    print('Cannot found RT-Thread root directory, please check RTT_ROOT')
    print(RTT_ROOT)
    sys.exit(-1)

TARGET = 'rt-thread.elf'
PROJECT_DIR = os.path.abspath(os.getcwd())

DefaultEnvironment(tools=[])
env = Environment(tools = ['mingw'],
    AS = rtconfig.AS, ASFLAGS = rtconfig.AFLAGS,
    CC = rtconfig.CC, CCFLAGS = rtconfig.CFLAGS,
    AR = rtconfig.AR, ARFLAGS = '-rc',
    CXX = rtconfig.CXX, CXXFLAGS = rtconfig.CXXFLAGS,
    LINK = rtconfig.LINK, LINKFLAGS = rtconfig.LFLAGS)
env.PrependENVPath('PATH', rtconfig.EXEC_PATH)
env.AppendUnique(CPPDEFINES = [])

Export('RTT_ROOT')
Export('rtconfig')

if os.path.exists('libraries'):
    libraries_path_prefix = 'libraries'
else:
    libraries_path_prefix = '../../libraries'

SDK_LIB = libraries_path_prefix
Export('SDK_LIB')

objs = PrepareBuilding(env, RTT_ROOT, has_libcpu=False)
# Re-import Env after PrepareBuilding sets it (from building import * captured None)
import building
building_Env = building.Env

# Add project root to include paths so lv_conf.h can be found by LVGL
env.AppendUnique(CPPPATH=[PROJECT_DIR])

stm32_library = 'STM32H7xx_HAL'
rtconfig.BSP_LIBRARY_TYPE = stm32_library

if not os.path.exists('libraries'):
    objs.extend(SConscript(os.path.join(libraries_path_prefix, 'SConscript')))

# ============================================================================
# Compilation Database for clangd (compile_commands.json)
# ============================================================================
env.Tool('compilation_db')
env['COMPILATIONDB_USE_ABSPATH'] = True
db_path = os.path.abspath('compile_commands.json')
db_target = env.CompilationDatabase(db_path)
env.NoClean(db_target)

# ============================================================================
# FIX: collect2.exe spawn failure on Windows
#
# SPAWN override: use subprocess.Popen for compilation normally.
# For the link step, run gcc via a child Python process to avoid the
# collect2 CreateProcess bug.
# ============================================================================
GCC_PATH = os.path.join(rtconfig.EXEC_PATH, 'arm-none-eabi-gcc.exe')
_PYTHON = os.environ['PYTHON_PATH']

def _link_via_child_python(cmd_args):
    """Run gcc link command in a child Python process (clean process state).

    Extract .o paths from scons' args list and deduplicate them.  Packages
    (e.g. u8g2) may compile .o into the source tree outside build/, so we
    MUST NOT rely on scanning build/ alone.

    To avoid Windows' MAX_PATH/cmd-line-length limit (~8191 chars), write
    the .o list into a GCC response file and pass it via @file.rsp.
    """
    # Collect unique .o paths from SCons' args (may be quoted)
    obj_paths = sorted(set(str(a).strip('"') for a in cmd_args if str(a).strip('"').endswith('.o')))

    if not obj_paths:
        sys.stderr.write("ERROR: No .o files in link args\n")
        return 1

    # Deduplicate: prefer build/ copies, drop source-tree duplicates
    # (e.g. startup_stm32h750xx.o appears in both build/board/ and
    #  libraries/.../gcc/ — only the build/ variant should be linked)
    seen_basenames = set()
    unique_paths = []
    for p in obj_paths:
        bn = os.path.basename(p)
        if bn in seen_basenames:
            if p.startswith('build'):
                # Replace the earlier non-build entry with this build/ one
                for i, prev in enumerate(unique_paths):
                    if os.path.basename(prev) == bn:
                        unique_paths[i] = p
                        break
        else:
            seen_basenames.add(bn)
            unique_paths.append(p)
    obj_paths = sorted(unique_paths)

    # Use forward slashes in the response file (GCC/Windows accepts them,
    # and avoids backslash-as-escape issues in the response-file parser)
    rsp = os.path.join(PROJECT_DIR, '.link_objs.rsp')
    with open(rsp, 'w', newline='\n') as f:
        for p in obj_paths:
            f.write(p.replace('\\', '/') + '\n')

    helper = os.path.join(PROJECT_DIR, '.link_helper.py')
    with open(helper, 'w', newline='\n') as f:
        f.write("import subprocess, os, sys\n")
        f.write("gcc = %r\n" % GCC_PATH)
        f.write("target = %r\n" % TARGET)
        f.write("lflags = %r\n" % rtconfig.LFLAGS)
        f.write("rsp = %r\n" % rsp)
        f.write("link_env = os.environ.copy()\n")
        f.write("link_env['PATH'] = %r + os.pathsep + r'C:\\Windows\\system32' + os.pathsep + r'C:\\Windows'\n" % rtconfig.EXEC_PATH)
        # Count objects
        f.write("with open(rsp) as _f:\n")
        f.write("    objs = [l.strip() for l in _f if l.strip()]\n")
        f.write("sys.stderr.write('Linking ' + target + ' with ' + str(len(objs)) + ' objects...\\n')\n")
        f.write("cmd = [gcc] + lflags.split() + ['-o', target, '@' + rsp]\n")
        f.write("result = subprocess.run(cmd, env=link_env, shell=False)\n")
        f.write("if result.returncode != 0:\n")
        f.write("    sys.stderr.write('LINK FAILED (rc=' + str(result.returncode) + ')\\n')\n")
        f.write("sys.exit(result.returncode)\n")
    result = subprocess.run([_PYTHON, helper], env=os.environ, cwd=PROJECT_DIR)
    try: os.remove(helper)
    except: pass
    try: os.remove(rsp)
    except: pass
    return result.returncode

def _spawn_with_link_fix(sh, escape, cmd, args, env):
    """SPAWN: compile normally; for link step, dispatch to child Python process."""
    if cmd == 'del':
        for f in args[1:]:
            try: os.remove(f)
            except: pass
        return 0

    # Detect link step: gcc/g++ invoked without -c flag (compilation has -c)
    is_link = False
    if args and any(x in str(args[0]) for x in ('gcc', 'g++', 'ld')):
        has_minus_c = any(str(a).strip() == '-c' for a in args)
        if not has_minus_c and any('.elf' in str(a) for a in args):
            is_link = True
    if is_link:
        sys.stderr.write("Linking via child Python process...\n")
        try:
            rc = _link_via_child_python(args)
        except Exception as e:
            sys.stderr.write("LINK FAILED (exception: %s)\n" % traceback.format_exc())
            return 1
        if rc != 0:
            sys.stderr.write("LINK FAILED (rc=%d)\n" % rc)
        return rc

    # Compilation: standard subprocess.Popen (string-based approach)
    _e = {}
    for k, v in env.items():
        try: _e[k] = str(v)
        except: pass
    old_path = os.environ.get('PATH', '')
    try:
        os.environ['PATH'] = rtconfig.EXEC_PATH + os.pathsep + _e.get('PATH', old_path)
        newargs = ' '.join(str(a) for a in args[1:])
        cmdline = str(args[0]) + ' ' + newargs
        proc = subprocess.Popen(cmdline, env=_e, shell=False)
        return proc.wait()
    except OSError as e:
        print('Error in calling command: ' + cmd)
        return getattr(e, 'errno', -1)
    finally:
        os.environ['PATH'] = old_path

env['SPAWN'] = _spawn_with_link_fix
env['PSPAWN'] = _spawn_with_link_fix

DoBuilding(TARGET, objs)

# Make all targets default (program + compilation database)
if building_Env is not None and building_Env.get('target'):
    env.Default(building_Env['target'], db_target)
else:
    env.Default(db_target)
