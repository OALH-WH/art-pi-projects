import os
import sys
import subprocess
import rtconfig

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
_PYTHON = r'C:\Python314\python.exe'

def _link_via_child_python(cmd_args):
    """Run gcc link command in a child Python process (clean process state).

    We scan build/ for .o files instead of using scons' args list, which has
    duplicated entries (575 vs ~93 actual .o files).
    """
    # Scan build/ for actual .o files
    obj_paths = []
    for root, dirs, files in os.walk('build'):
        for f in files:
            if f.endswith('.o'):
                obj_paths.append(os.path.join(root, f))
    obj_paths.sort()

    if not obj_paths:
        sys.stderr.write("ERROR: No .o files found in build/\n")
        return 1

    helper = os.path.join(PROJECT_DIR, '.link_helper.py')
    with open(helper, 'w', newline='\n') as f:
        f.write("import subprocess, os, sys\n")
        f.write("gcc = %r\n" % GCC_PATH)
        f.write("target = %r\n" % TARGET)
        f.write("lflags = %r\n" % rtconfig.LFLAGS)
        f.write("objs = %r\n" % obj_paths)
        f.write("link_env = os.environ.copy()\n")
        f.write("link_env['PATH'] = %r + os.pathsep + r'C:\\Windows\\system32' + os.pathsep + r'C:\\Windows'\n" % rtconfig.EXEC_PATH)
        f.write("cmd = [gcc] + lflags.split() + ['-o', target] + objs\n")
        f.write("sys.stderr.write('Linking ' + target + ' with ' + str(len(objs)) + ' objects...\\n')\n")
        f.write("result = subprocess.run(cmd, env=link_env, shell=False)\n")
        f.write("if result.returncode != 0:\n")
        f.write("    sys.stderr.write('LINK FAILED (rc=' + str(result.returncode) + ')\\n')\n")
        f.write("sys.exit(result.returncode)\n")
    result = subprocess.run([_PYTHON, helper], env=os.environ, cwd=PROJECT_DIR)
    try: os.remove(helper)
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
        rc = _link_via_child_python(args)
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
