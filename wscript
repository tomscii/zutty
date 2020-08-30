#! ./waf
# encoding: utf-8

import subprocess

top = '.'
out = 'build'

def options(opt):
    opt.add_option(
        '--debug',
        action='store_true',
        default=False,
        dest='debug',
        help='Compile-in debug facilities')

    opt.add_option(
        '--no-werror',
        action='store_false',
        default=True,
        dest='werror',
        help='Treat warnings as errors')

    opt.recurse('src')

def configure(cfg):
    vsn_cmd = "git describe --tags --dirty 2>/dev/null || cat version.txt";
    vsn = subprocess.check_output(vsn_cmd, shell=True).strip('\r\n')
    with open("version.txt", 'w') as f: f.write(vsn + '\n')
    cfg.msg('Zutty version', vsn)
    cfg.env.append_value('CXXFLAGS', ['-DZUTTY_VERSION=\"' + vsn + '\"'])

    cfg.msg('Debug build', "yes" if cfg.options.debug else "no")

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

    if cfg.options.debug:
        cfg.options.werror = False
        cfg.env.append_value('CXXFLAGS', ['-DDEBUG'])

    if cfg.options.werror:
        cfg.env.append_value('CXXFLAGS', ['-Werror'])

    cfg.check_cfg(package='freetype2', args=['--cflags', '--libs'],
                  uselib_store='FT')

    cfg.check_cfg(package='xmu', args=['--cflags', '--libs'],
                  uselib_store='XMU')

    cfg.recurse('src')

def build(bld):
    bld.recurse('src')
