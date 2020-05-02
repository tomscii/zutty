#! ./waf
# encoding: utf-8

top = '.'
out = 'build'

def options(opt):
    opt.add_option(
        '--no-werror',
        action='store_false',
        default=True,
        dest='werror',
        help='Treat warnings as errors')

    opt.recurse('src')

def configure(cfg):
    cfg.env.append_value('CXXFLAGS',
       ['-Wall',
        '-Wextra',
        '-Wno-unused-parameter',
        '-Wsign-compare',
        '-std=c++14',
        '-g', '-ggdb',
        '-O2', '-march=native',
        '-fPIC', '-fsigned-char'])

    if cfg.options.werror:
        cfg.env.append_value('CFLAGS', ['-Werror'])
        cfg.env.append_value('CXXFLAGS', ['-Werror'])

    cfg.check_cfg(package='freetype2', args=['--cflags', '--libs'],
                  uselib_store='FT')

    cfg.recurse('src')

def build(bld):
    bld.recurse('src')
