cflags = [
  '-DPREFIX="@0@"'.format(byx_prefix),
  '-DCONFDIR="@0@"'.format(byx_confdir),
  '-DBINDIR="@0@"'.format(byx_bindir),
  '-DSBINDIR="@0@"'.format(byx_sbindir),
  '-DLIBDIR="@0@"'.format(byx_libdir),
  '-DLIBEXECDIR="@0@"'.format(byx_libexecdir),
  '-DPLUGINDIR="@0@"'.format(byx_plugindir),
  '-DDATADIR="@0@"'.format(byx_datadir),
  '-DLOCALEDIR="@0@"'.format(byx_localedir),
  '-DRUNDIR="@0@"'.format(byx_rundir),
  '-DVARDIR="@0@"'.format(byx_vardir),
]

ldflags = [
  '-rdynamic'
]

sources = files(
  'main.c',
  'util/byx-glib-keyfile.c',
  'config/byx-run-data.c',
  'config/byx-persist-data.c',
  'config/byx-config-manager.c',
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
