# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.1'
APPNAME='simple-chrono-chat'

from waflib import Configure, Utils, Logs, Context
import os

def options(opt):
    # gnu_dirs: Sets various standard variables such as INCLUDEDIR
    opt.load(['compiler_cxx', 'gnu_dirs'])

    opt.load(['default-compiler-flags', 'boost'],
              tooldir=['.waf-tools'])


def configure(conf):
    conf.load(['compiler_cxx', 'default-compiler-flags', 'boost', 'gnu_dirs'])

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    conf.check_cfg (package='ChronoSync', args=['ChronoSync >= 0.1', '--cflags', '--libs'],
                    uselib_store='SYNC', mandatory=True)

    boost_libs = 'system random thread filesystem'

    conf.check_boost(lib=boost_libs)

    conf.write_config_header('config.h')

def build (bld):
    for i in bld.path.ant_glob(['src/*.cpp']):
        name = str(i)[:-len(".cpp")]
        bld(features=['cxx', 'cxxprogram'],
            target="bin/%s" % name,
            source=[i] + bld.path.ant_glob(['%s/**/*.cpp' % name]),
            use='NDN_CXX BOOST SYNC'
        )
