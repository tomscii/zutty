#! ./waf
# encoding: utf-8

import subprocess, sys

top = '.'
out = 'build'

def options(opt):
    opt.add_option('--libpath-egl', action='store', default=False,
                   help='Path to libEGL.so')

    opt.add_option('--libpath-gles', action='store', default=False,
                   help='Path to libGLESv2.so')

    opt.add_option('--libpath-pthread', action='store', default=False,
                   help='Path to libpthread.so')

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
    cfg.env.append_value('CXXFLAGS', ['-DZUTTY_VERSION=\"' + vsn + '\"'])

    cfg.msg('Debug build', "yes" if cfg.options.debug else "no")

    cfg.load('compiler_cxx')

    cfg.env.append_value('CXXFLAGS',
       ['-Wall',
        '-Wextra',
        '-Wno-unused-parameter',
        '-Wsign-compare',
        '-std=c++14',
        '-g', '-ggdb',
        '-O2', '-march=native',
        '-fno-omit-frame-pointer',
        '-fPIC', '-fsigned-char'])

    cfg.env.target = 'zutty'
    if cfg.options.debug:
        cfg.env.target = 'zutty.dbg'
        cfg.options.werror = False
        cfg.env.append_value('CXXFLAGS', ['-DDEBUG'])

    if cfg.options.werror:
        cfg.env.append_value('CXXFLAGS', ['-Werror'])

    default_libpath = cmd ("echo /usr/lib/$(gcc -dumpmachine)")
    cfg.env.LIB_EGL     = ['EGL']
    cfg.env.LIBPATH_EGL = [cfg.options.libpath_egl
                           if cfg.options.libpath_egl
                           else default_libpath]
    cfg.env.LIB_GLES     = ['GLESv2']
    cfg.env.LIBPATH_GLES = [cfg.options.libpath_gles
                            if cfg.options.libpath_gles
                            else default_libpath]
    cfg.env.LIB_THREAD     = ['pthread']
    cfg.env.LIBPATH_THREAD = [cfg.options.libpath_pthread
                              if cfg.options.libpath_pthread
                              else default_libpath]

    cfg.check_cfg(package='freetype2', args=['--cflags', '--libs'],
                  uselib_store='FT')

    cfg.check_cfg(package='xmu', args=['--cflags', '--libs'],
                  uselib_store='XMU')

    cfg.check(header_name='EGL/egl.h', features='cxx')
    cfg.check(header_name='GLES3/gl31.h', features='cxx')

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
