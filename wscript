#! ./waf
# encoding: utf-8

import subprocess, sys
from waflib import Logs

top = '.'
out = 'build'

def options(opt):
    opt.add_option('--debug', action='store_true', default=False,
                   dest='debug', help='Compile-in debug facilities')

    opt.add_option('--no-werror', action='store_false', default=True,
                   dest='werror', help='Treat warnings as errors')

    opt.load('compiler_cxx')

    opt.recurse('src')

def configure(cfg):
    vsn = cmd("(git checkout -- version.txt && " +
              "git describe --tags --dirty) 2>/dev/null || cat version.txt")
    with open("version.txt", 'w') as f: f.write(vsn + '\n')
    cfg.msg('Zutty version', vsn)
    if cfg.options.debug:
        vsn = vsn + '-DEBUG'

    cfg.msg('Debug build', "yes" if cfg.options.debug else "no")

    cfg.load('compiler_cxx')

    cfg.env.append_value('CXXFLAGS',
       ['-std=c++14',
        '-fno-omit-frame-pointer',
        '-fsigned-char',
        '-Wall',
        '-Wextra',
        '-Wsign-compare',
        '-Wno-unused-parameter',
        '-DZUTTY_VERSION="{}"'.format (vsn)
        ])

    platform = cmd("uname -s")
    cfg.msg('Target platform', platform)
    if platform == 'Linux':
        cfg.env.append_value('CXXFLAGS', ['-DLINUX'])
    elif platform == 'FreeBSD':
        cfg.env.append_value('CXXFLAGS',
                             ['-DBSD', '-DFREEBSD', '-I/usr/local/include'])
        cfg.env.append_value('LINKFLAGS', ['-L/usr/local/lib'])
    elif platform == 'OpenBSD':
        cfg.env.append_value('CXXFLAGS',
                             ['-DBSD', '-DOPENBSD', '-I/usr/X11R6/include'])
        cfg.env.append_value('LINKFLAGS', ['-L/usr/X11R6/lib'])
    elif platform == 'NetBSD':
        cfg.env.append_value('CXXFLAGS', ['-DBSD', '-DNETBSD'])
    elif platform == 'Darwin':
        cfg.env.append_value('CXXFLAGS', ['-DMACOS'])
    elif platform == 'SunOS':
        cfg.env.append_value('CXXFLAGS', ['-DSOLARIS'])
    elif platform == 'GNU/kFreeBSD':
        cfg.env.append_value('CXXFLAGS', ['-DBSD'])
    elif platform == 'GNU':
        cfg.env.append_value('CXXFLAGS', ['-DGNU'])
    else:
        Logs.error ('Unknown platform: {}'.format (platform))
        sys.exit (1)

    if cfg.options.debug:
        cfg.env.target = 'zutty.dbg'
        cfg.env.append_value('CXXFLAGS',
           ['-DDEBUG', '-Og', '-g', '-ggdb'])
    else:
        cfg.env.target = 'zutty'
        cfg.env.append_value('CXXFLAGS',
           ['-Werror', '-O3', '-flto'])
        cfg.env.append_value('LINKFLAGS',
           ['-flto'])

    cfg.check_cfg(package='freetype2', args=['--cflags', '--libs'],
                  uselib_store='FT')

    cfg.check_cfg(package='xmu', args=['--cflags', '--libs'],
                  uselib_store='XMU')

    cfg.check_cxx(header_name='EGL/egl.h')
    cfg.check_cxx(header_name='GLES3/gl31.h')
    cfg.check_cxx(lib='EGL', uselib_store='EGL')
    cfg.check_cxx(lib='GLESv2', uselib_store='GLES')
    cfg.check_cxx(lib='pthread', uselib_store='THREAD')

    cfg.recurse('src')

def build(bld):
    bld.recurse('src')

def cmd(s):
    # Try to cover all relevant Python versions (2.7 - 3.8+)
    if (100 * sys.version_info.major + sys.version_info.minor >= 305):
        proc = subprocess.run(s, shell=True, check=True,
                              stdout=subprocess.PIPE, universal_newlines=True)
        return proc.stdout.strip('\r\n')
    else:
        return subprocess.check_output(s, shell=True).strip('\r\n')
