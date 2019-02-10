cflags = [
  '-DBYX_CONFDIR="@0@"'.format(byx_confdir),
  '-DBYX_BINDIR="@0@"'.format(byx_bindir),
  '-DBYX_SBINDIR="@0@"'.format(byx_sbindir),
  '-DBYX_LIBDIR="@0@"'.format(byx_libdir),
  '-DBYX_PLUGINDIR="@0@"'.format(byx_plugindir),
  '-DBYX_LIBEXECDIR="@0@"'.format(byx_libexecdir),
  '-DBYX_DATADIR="@0@"'.format(byx_datadir),
  '-DBYX_LOCALEDIR="@0@"'.format(byx_localedir),
  '-DBYX_RUNDIR="@0@"'.format(byx_rundir),
  '-DBYX_VARDIR="@0@"'.format(byx_vardir),
]

ldflags = [
  '-rdynamic'
]

sources = files(
  'main.c',
  'main-utils.c',
  'util/util-glib-keyfile.c',
  'config/byx-config.c',
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
