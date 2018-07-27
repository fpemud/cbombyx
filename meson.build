project('Bombyx', 'c',
                version: '1.13.1',
                license: 'GPL2+',
        default_options: [
          'buildtype=debugoptimized',
          'c_std=gnu99'
        ],
          meson_version: '>= 0.44.0'
)

nm_name = meson.project_name()

nm_version = meson.project_version()
byx_major_version = nm_version.split('.')[0].to_int()
byx_minor_version = nm_version.split('.')[1].to_int()
byx_micro_version = nm_version.split('.')[2].to_int()

byx_gir_version = '1.0'

byx_prefix = get_option('prefix')
byx_bindir = join_paths(byx_prefix, get_option('bindir'))
byx_datadir = join_paths(byx_prefix, get_option('datadir'))
byx_includedir = join_paths(byx_prefix, get_option('includedir'))
byx_libdir = join_paths(byx_prefix, get_option('libdir'))
byx_libexecdir = join_paths(byx_prefix, get_option('libexecdir'))
byx_localedir = join_paths(byx_prefix, get_option('localedir'))
byx_localstatedir = join_paths(byx_prefix, get_option('localstatedir'))
byx_mandir = join_paths(byx_prefix, get_option('mandir'))
byx_runstatedir = join_paths(byx_localstatedir, 'run')
byx_sbindir = join_paths(byx_prefix, get_option('sbindir'))
byx_sysconfdir = join_paths(byx_prefix, get_option('sysconfdir'))

nm_pkgsbindir = join_paths(byx_sbindir, nm_name)
nm_pkgconfdir = join_paths(byx_sysconfdir, nm_name)
nm_pkgdatadir = join_paths(byx_datadir, nm_name)
nm_pkgincludedir = join_paths(byx_includedir, nm_name)
nm_pkglibdir = join_paths(byx_prefix, 'lib', nm_name)
nm_pkgrundir = join_paths(byx_runstatedir, nm_name)
nm_pkgstatedir = join_paths(byx_localstatedir, nm_name)
nm_vpndir = join_paths(byx_libdir, nm_name)
nm_plugindir = join_paths(byx_libdir, nm_name, nm_version)

nm_build_cflags = [
  '-DNM_BUILD_SRCDIR="@0@"'.format(meson.source_root()),
  '-DNM_BUILD_BUILDDIR="@0@"'.format(meson.build_root()),
]

libnm_name = 'libnm'

nm_debug = get_option('buildtype').contains('debug')

cc = meson.get_compiler('c')

config_h = configuration_data()

# defines
config_h.set_quoted('GETTEXT_PACKAGE', nm_name)
config_h.set_quoted('PACKAGE_STRING', '@0@ @1@'.format(nm_name, nm_version))
config_h.set_quoted('VERSION', nm_version)

# headers
use_sys_random = cc.has_function('getrandom', prefix: '#include <sys/random.h>')
config_h.set10('USE_SYS_RANDOM_H', use_sys_random)
config_h.set10('HAVE_GETRANDOM', use_sys_random or cc.has_function('getrandom', prefix: '#include <linux/random.h>'))

# compiler flags
common_flags = []
common_ldflags = []

enable_ld_gc = get_option('ld_gc')
if enable_ld_gc
  test_cflags = [
    '-fdata-sections',
    '-ffunction-sections',
  ]

  test_ldflags = ['-Wl,--gc-sections']

  foreach cflag: test_cflags + test_ldflags
    assert(cc.has_argument(cflag), 'Unused symbol eviction requested but not supported. Use -Dld_gc=false to build without it.')
  endforeach

  common_flags += test_cflags
  common_ldflags += test_ldflags
endif

if nm_debug
  test_cflags = [
    '-fno-strict-aliasing',
    '-Wdeclaration-after-statement',
    '-Wfloat-equal',
    '-Wimplicit-fallthrough',
    '-Winit-self',
    '-Wlogical-op',
    '-Wmissing-declarations',
    '-Wmissing-include-dirs',
    '-Wmissing-prototypes',
    '-Wno-duplicate-decl-specifier',
    '-Wno-format-truncation',
    '-Wno-missing-braces',
    '-Wno-missing-field-initializers',
    '-Wno-pragmas',
    '-Wno-sign-compare',
    '-Wno-unused-parameter',
    '-Wparentheses-equality',
    '-Wpointer-arith',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wtypedef-redefinition',
    '-Wundef',
    '-Wunknown-attributes'
  ]
  common_flags += cc.get_supported_arguments(test_cflags)
endif

add_project_arguments(common_flags, language: 'c')
add_project_link_arguments(common_ldflags, language: 'c')

uuid_dep = dependency('uuid')
libelogind_dep = dependency('libelogind', version: '>= 219', required: false)
libudev_dep = dependency('libudev', version: '>= 175')
dbus_dep = dependency('dbus-1', version: '>= 1.1')
libndp_dep = dependency('libndp')

libsystemd_dep = dependency('libsystemd', version: '>= 209', required: false)

systemd_dep = dependency('systemd', required: false)

gio_unix_dep = dependency('gio-unix-2.0', version: '>= 2.40')

glib_dep = declare_dependency(
  dependencies: [
    gio_unix_dep,
    dependency('gmodule-2.0')
  ],
  compile_args: [
    '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_40',
    '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_40'
  ]
)

if run_command('test', '-e', '/etc/sysconfig/network-scripts').returncode() == 0
  distro = 'redhat'
elif run_command('test', '-e', '/etc/SuSE-release').returncode() == 0
  distro = 'suse'
elif run_command('test', '-e', '/etc/debian_version').returncode() == 0
  distro = 'debian'
elif run_command('test', '-e', '/etc/gentoo-release').returncode() == 0
  distro = 'gentoo'
else
  distro = 'unknown'
endif

# introspection support
enable_introspection = get_option('introspection')
if enable_introspection
  gir_dep = dependency('gobject-introspection-1.0', version: '>= 0.9.6', required: false)
  assert(gir_dep.found(), 'introspection support was requested, but the gobject-introspection library is not available. Use -Dintrospection=false to build without it.')
endif

systemd_system_unit_dir = get_option('systemdsystemunitdir')
install_systemd_unit_dir = (systemd_system_unit_dir != 'no')

if install_systemd_unit_dir and systemd_system_unit_dir == ''
  assert(systemd_dep.found(), 'systemd required but not found, please provide a valid systemd user unit dir or disable it')
  systemd_system_unit_dir = systemd_dep.get_pkgconfig_variable('systemdsystemunitdir')
endif
config_h.set10('HAVE_SYSTEMD', install_systemd_unit_dir)

enable_systemd_journal = get_option('systemd_journal')
if enable_systemd_journal
  assert(libsystemd_dep.found(), 'Missing systemd-journald support')
endif
config_h.set10('SYSTEMD_JOURNAL', enable_systemd_journal)

config_logging_backend_default = get_option('config_logging_backend_default')
if config_logging_backend_default == 'default'
  config_logging_backend_default = (enable_systemd_journal ? 'journal' : 'syslog')
endif
config_h.set_quoted('BYX_CONFIG_DEFAULT_LOGGING_BACKEND', config_logging_backend_default)

enable_suspend_resume = get_option('suspend_resume')
suspend_resume = ''

if enable_suspend_resume
  if libsystemd_dep.found()
    system_inhibit_dep = libsystemd_dep
    suspend_resume = 'systemd'
    config_h.set('SUSPEND_RESUME_SYSTEMD', true)
  else
    system_inhibit_dep = dependency('libsystemd-login', version: '>= 183', required: false)
    if system_inhibit_dep.found()
      suspend_resume = 'systemd'
      config_h.set('SUSPEND_RESUME_SYSTEMD', true)
    elif libelogind_dep.found()
      system_inhibit_dep = libelogind_dep
      suspend_resume = 'elogind'
      config_h.set('SUSPEND_RESUME_ELOGIND', true)
    else
      suspend_resume = 'upower'
      config_h.set('SUSPEND_RESUME_UPOWER', true)
    endif
  endif
endif

# crypto
crypto = get_option('crypto')
if crypto == 'nss'
  crypto_dep = dependency('nss')
else
  crypto_dep = dependency('gnutls', version: '>= 2.12')
endif

dbus_conf_dir = get_option('dbus_conf_dir')
if dbus_conf_dir == ''
  assert(dbus_dep.found(), 'D-Bus required but not found, please provide a valid system bus config dir')
  dbus_conf_dir = join_paths(dbus_dep.get_pkgconfig_variable('sysconfdir'), 'dbus-1', 'system.d')
endif

dbus_ifaces_dir = get_option('dbus_ifaces_dir')
if dbus_ifaces_dir == ''
  assert(dbus_dep.found(), 'D-Bus required but not found, please provide a valid interfaces dir')
  dbus_ifaces_dir = dbus_dep.get_pkgconfig_variable('interfaces_dir')
endif

config_dns_rc_manager_default = get_option('config_dns_rc_manager_default')

config_h.set_quoted('BYX_CONFIG_DEFAULT_MAIN_RC_MANAGER', config_dns_rc_manager_default)

# iptables path
config_h.set_quoted('IPTABLES_PATH', find_program(get_option('iptables')).path())

# dnsmasq path
config_h.set_quoted('DNSMASQ_PATH', find_program(get_option('dnsmasq')).path())

# dnssec-trigger-script path
dnssec_trigger_script = find_program(get_option('dnssec_trigger'), required: false)
config_h.set_quoted('DNSSEC_TRIGGER_SCRIPT', (dnssec_trigger_script.found() ? dnssec_trigger_script.path() : '/usr/libexec/dnssec-trigger-script'))

# system CA certificates path
system_ca_path = get_option('system_ca_path')
config_h.set_quoted('SYSTEM_CA_PATH', system_ca_path)

enable_libpsl = get_option('libpsl')
if enable_libpsl
  libpsl_dep = dependency('libpsl', version: '>= 0.1')
endif
config_h.set10('WITH_LIBPSL', enable_libpsl)

enable_concheck = get_option('concheck')
if enable_concheck
  libcurl_dep = dependency('libcurl', version: '>= 7.24.0', required: false)
  assert(libcurl_dep.found(), 'concheck requires libcurl library. Use -Dconcheck=false to disable it')
endif
config_h.set10('WITH_CONCHECK', enable_concheck)

more_asserts = get_option('more_asserts')
if more_asserts == 'no'
  more_asserts = 0
elif more_asserts == 'all'
  more_asserts = 100
else
  more_asserts = more_asserts.to_int()
endif
config_h.set('NM_MORE_ASSERTS', more_asserts)

more_logging = get_option('more_logging')
config_h.set10('NM_MORE_LOGGING', more_logging)

generic_support_src = 'int main() { int a = 0; int b = _Generic (a, int: 4); return b + a; };'
config_h.set10('_NM_CC_SUPPORT_GENERIC', cc.compiles(generic_support_src))

auto_support_src = 'int main() { int a = 0; __auto_type b = a; return b + a; };'
config_h.set10('_NM_CC_SUPPORT_AUTO_TYPE', cc.compiles(auto_support_src))

# Vala bindings
enable_vapi = get_option('vapi')
if enable_vapi
  vala_req_version = '>= 0.17.1.24'
  assert(add_languages('vala', required: false), 'vala is required to build. Use -Dvapi=false to disable it')
  assert(meson.get_compiler('vala').version().version_compare(vala_req_version), 'vala ' + vala_req_version + ' is required to build. Use -Dvapi=false to disable it')
endif

# Tests, utilities and documentation
tests = get_option('tests')
enable_tests = (tests != 'no')
require_root_tests = (tests == 'root')
test_script = find_program(join_paths(meson.source_root(), 'tools', 'run-nm-test.sh'))

# valgrind
locations = get_option('valgrind')
enable_valgrind = (locations != ['no'])
if enable_valgrind
  valgrind = find_program(locations, required: false)
  enable_valgrind = valgrind.found()
endif

if enable_valgrind
  valgrind_suppressions_path = get_option('valgrind_suppressions')
  if valgrind_suppressions_path == ''
    valgrind_suppressions_path = join_paths(meson.source_root(), 'valgrind.suppressions')
  endif
endif

test_args = [
  '--called-from-make',
  meson.build_root(),
  '',
  enable_valgrind ? valgrind.path() : '',
  enable_valgrind ? valgrind_suppressions_path : '',
  '--launch-dbus=auto'
]

py3 = import('python3')
python = py3.find_python()

if python.found()
  config_h.set_quoted('TEST_NM_PYTHON', python.path())
endif

# check if we can build setting property documentation
'''
build_docs=no
if test -n "$INTROSPECTION_MAKEFILE"; then
  # If g-i is installed we know we have python, but we might not have pygobject
  if ! "$PYTHON" -c 'from gi.repository import GObject' >& /dev/null; then
    AC_MSG_ERROR(["--enable-introspection aims to build the settings documentation. This requires GObject introspection for python (pygobject)])
  fi

  AC_PATH_PROG(PERL, perl)
  if test -z "$PERL"; then
    AC_MSG_ERROR([--enable-introspection requires perl])
  fi
  AC_PATH_PROG(XSLTPROC, xsltproc)
  if test -z "$XSLTPROC"; then
    AC_MSG_ERROR([--enable-introspection requires xsltproc])
  fi

  have_introspection=yes
  if test "$enable_gtk_doc" = "yes"; then
    build_docs=yes
  fi
else
  if test "$enable_gtk_doc" = "yes"; then
    # large parts of the documentation require introspection/pygobject to extract
    # the documentation out of the source files. You cannot enable gtk-doc without alone.
    AC_MSG_ERROR(["--with-gtk-doc requires --enable-introspection"])
  fi
  have_introspection=no
fi
'''

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

po_dir = join_paths(meson.source_root(), 'po')

intltool_merge = find_program('intltool-merge')
intltool_cache = join_paths(po_dir, '.intltool-merge-cache')
intltool_desktop_cmd = [intltool_merge, '-d', '-u', '-c', intltool_cache, po_dir, '@INPUT@', '@OUTPUT@']
intltool_xml_cmd = [intltool_merge, '-x', '-u', '-c', intltool_cache, po_dir, '@INPUT@', '@OUTPUT@']

perl = find_program('perl')
xsltproc = find_program('xsltproc')

check_exports = find_program(join_paths(meson.source_root(), 'tools', 'check-exports.sh'))

content_files = []

# FIXME: every plugin calls to this:
'''
set_sanitizer_env = \
        [ -n "$(SANITIZER_ENV)" ] && export $(SANITIZER_ENV) ; \
        if [ -n "$(1)" ] && echo $(CFLAGS) | grep -e -fsanitize=address ; then \
                export LD_PRELOAD="$${LD_PRELOAD}:$$(ldd $(1) | grep libasan\.so\.. -o | head -n 1)"; \
        fi

check_so_symbols = \
        $(call set_sanitizer_env,$(1)); \
        LD_BIND_NOW=1 LD_PRELOAD=$${LD_PRELOAD}:$(1) $(builddir)/src/NetworkManager --version >/dev/null
'''

top_inc = include_directories('.')

subdir('introspection')
subdir('shared')
subdir('src')
subdir('data')
subdir('po')

if enable_vapi
  subdir('vapi')
endif

subdir('examples/C/glib')

enable_qt = get_option('qt')
if enable_qt
  add_languages('cpp')
  subdir('examples/C/qt')
endif

enable_docs = get_option('docs')
if enable_docs
  subdir('man')

  #assert(enable_introspection, '-Dgtk_doc=true requires -Dintrospection=true')

  subdir('docs')
endif

config = 'config.h'

configure_file(
  input: config + '.meson',
  output: config,
  configuration: config_h
)

meson.add_install_script(
  'meson_post_install.py',
  byx_datadir,
  byx_bindir,
  nm_pkgconfdir,
  nm_pkglibdir,
  byx_localstatedir,
  enable_docs ? 'install_docs' : '',
  byx_mandir
)

output = '\nSystem paths:\n'
output += '  prefix: ' + byx_prefix + '\n'
output += '  exec_prefix: ' + byx_prefix + '\n'
output += '  systemdunitdir: ' + systemd_system_unit_dir + '\n'
output += '  nmbinary: ' + nm_pkgsbindir + '\n'
output += '  nmconfdir: ' + nm_pkgconfdir + '\n'
output += '  nmlibdir: ' + nm_pkglibdir + '\n'
output += '  nmdatadir: ' + nm_pkgdatadir + '\n'
output += '  nmstatedir: ' + nm_pkgstatedir + '\n'
output += '  nmrundir: ' + nm_pkgrundir + '\n'
output += '  nmvpndir: ' + nm_vpndir + '\n'
output += '  nmplugindir: ' + nm_plugindir + '\n'
output += '\nPlatform:\n'
output += '  suspend/resume: ' + suspend_resume + '\n'
output += '\n'
output += '  systemd-journald: ' + enable_systemd_journal.to_string() + ' (default: logging.backend=' + config_logging_backend_default + ')\n'
endif
output += '\n'
output += '  concheck: ' + enable_concheck.to_string() + '\n'
output += '  config-dns-rc-manager-default: ' + config_dns_rc_manager_default + '\n'
output += '\n'
output += '\nMiscellaneous:\n'
output += '  have introspection: ' + enable_introspection.to_string() + '\n'
output += '  build documentation and manpages: ' + enable_docs.to_string() + '\n'
output += '  tests: ' + tests + '\n'
output += '  more-asserts: @0@\n'.format(more_asserts)
output += '  more-logging: ' + more_logging.to_string() + '\n'
output += '  warning-level: ' + get_option('warning_level') + '\n'
output += '  valgrind: ' + enable_valgrind.to_string()
if enable_valgrind
  output += ' ' + valgrind.path()
endif
output += '\n'
output += '  code coverage: ' + get_option('b_coverage').to_string() + '\n'
output += '  LTO: ' + get_option('b_lto').to_string() + '\n'
output += '  Linker garbage collection: ' + enable_ld_gc.to_string() + '\n'
output += '  sanitizers: ' + get_option('b_sanitize') + '\n'
output += '  Mozilla Public Suffix List: ' + enable_libpsl.to_string() + '\n'
message(output)
