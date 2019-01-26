cflags = [
  '-DPREFIX="@0@"'.format(byx_prefix),
  '-DBINDIR="@0@"'.format(byx_bindir),
  '-DDATADIR="@0@"'.format(byx_datadir),
  '-DLIBEXECDIR="@0@"'.format(byx_libexecdir),
  '-DLOCALSTATEDIR="@0@"'.format(byx_localstatedir),
  '-DRUNSTATEDIR="@0@"'.format(byx_runstatedir),
  '-DSBINDIR="@0@"'.format(byx_sbindir),
  '-DSYSCONFDIR="@0@"'.format(byx_sysconfdir),
  '-DRUNDIR="@0@"'.format(byx_pkgrundir),
  '-DNMCONFDIR="@0@"'.format(byx_pkgconfdir),
  '-DNMLOCALEDIR="@0@"'.format(byx_localedir),
  '-DNMPLUGINDIR="@0@"'.format(byx_plugindir),
  '-DNMRUNDIR="@0@"'.format(byx_pkgrundir),
  '-DNMSTATEDIR="@0@"'.format(byx_pkgstatedir),
  '-DNMLIBDIR="@0@"'.format(byx_pkglibdir)
]

ldflags = [
  '-rdynamic'
]

sources = files(
  'main.c',
  'util/byx-glib-keyfile.c',
  'config/byx-persist-data.c',




#  'config/byx-config-manager.c'
)

deps = [
  libudev_dep,
  liblogind_dep,
  glib_dep,
]

if enable_concheck
  deps += libcurl_dep
endif

subdir('common')

byx_daemon = executable(byx_name, sources,
  include_directories: top_inc,
         dependencies: deps,
               c_args: cflags,
            link_args: ldflags,
              install: true,
          install_dir: byx_sbindir
)
