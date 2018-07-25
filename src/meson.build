src_inc = include_directories('.')

install_data(
  'org.freedesktop.NetworkManager.conf',
  install_dir: dbus_conf_dir
)

subdir('systemd')

core_plugins = []

nm_cflags = ['-DNETWORKMANAGER_COMPILATION=NM_NETWORKMANAGER_COMPILATION_DAEMON']

nm_dep = declare_dependency(
  include_directories: src_inc,
  dependencies: nm_core_dep,
  compile_args: nm_cflags
)

cflags = nm_cflags + [
  '-DPREFIX="@0@"'.format(nm_prefix),
  '-DBINDIR="@0@"'.format(nm_bindir),
  '-DDATADIR="@0@"'.format(nm_datadir),
  '-DLIBEXECDIR="@0@"'.format(nm_libexecdir),
  '-DLOCALSTATEDIR="@0@"'.format(nm_localstatedir),
  '-DRUNSTATEDIR="@0@"'.format(nm_runstatedir),
  '-DSBINDIR="@0@"'.format(nm_sbindir),
  '-DSYSCONFDIR="@0@"'.format(nm_sysconfdir),
  '-DRUNDIR="@0@"'.format(nm_pkgrundir),
  '-DNMCONFDIR="@0@"'.format(nm_pkgconfdir),
  '-DNMLOCALEDIR="@0@"'.format(nm_localedir),
  '-DNMPLUGINDIR="@0@"'.format(nm_plugindir),
  '-DNMRUNDIR="@0@"'.format(nm_pkgrundir),
  '-DNMSTATEDIR="@0@"'.format(nm_pkgstatedir),
  '-DNMLIBDIR="@0@"'.format(nm_pkglibdir)
]

sources = files(
  'ndisc/nm-lndp-ndisc.c',
  'ndisc/nm-ndisc.c',
  'platform/nm-netlink.c',
  'platform/wifi/nm-wifi-utils-nl80211.c',
  'platform/wifi/nm-wifi-utils.c',
  'platform/wpan/nm-wpan-utils.c',
  'platform/nm-linux-platform.c',
  'platform/nm-platform.c',
  'platform/nm-platform-utils.c',
  'platform/nmp-netns.c',
  'platform/nmp-object.c',
  'main-utils.c',
  'NetworkManagerUtils.c',
  'nm-core-utils.c',
  'nm-dbus-object.c',
  'nm-dbus-utils.c',
  'nm-ip4-config.c',
  'nm-ip6-config.c',
  'nm-logging.c'
)

deps = [
  libsystemd_dep,
  libudev_dep,
  nm_core_dep
]

libnetwork_manager_base = static_library(
  nm_name + 'Base',
  sources: sources,
  dependencies: deps,
  c_args: cflags,
  link_with: libnm_core
)

sources = files(
  'devices/nm-device.c',
  'dns/nm-dns-dnsmasq.c',
  'dns/nm-dns-manager.c',
  'dns/nm-dns-plugin.c',
  'dns/nm-dns-systemd-resolved.c',
  'dns/nm-dns-unbound.c',
  'settings/plugins/keyfile/nms-keyfile-connection.c',
  'settings/plugins/keyfile/nms-keyfile-plugin.c',
  'settings/plugins/keyfile/nms-keyfile-reader.c',
  'settings/plugins/keyfile/nms-keyfile-utils.c',
  'settings/plugins/keyfile/nms-keyfile-writer.c',
  'settings/nm-agent-manager.c',
  'settings/nm-secret-agent.c',
  'settings/nm-settings.c',
  'settings/nm-settings-connection.c',
  'settings/nm-settings-plugin.c',
  'service/byx-service.c',
  'service/byx-service-manager.c',
  'nm-active-connection.c',
  'nm-act-request.c',
  'nm-audit-manager.c',
  'nm-auth-manager.c',
  'nm-auth-subject.c',
  'nm-auth-utils.c',
  'nm-dbus-manager.c',
  'nm-checkpoint.c',
  'nm-checkpoint-manager.c',
  'nm-config.c',
  'nm-config-data.c',
  'nm-connectivity.c',
  'nm-dcb.c',
  'nm-dhcp4-config.c',
  'nm-dhcp6-config.c',
  'nm-firewall-manager.c',
  'nm-hostname-manager.c',
  'nm-manager.c',
  'nm-netns.c',
  'nm-pacrunner-manager.c',
  'nm-policy.c',
  'nm-proxy-config.c',
  'nm-rfkill-manager.c',
  'nm-session-monitor.c',
  'nm-sleep-monitor.c'
)

deps = [
  dl_dep,
  libndp_dep,
  # FIXME: Some files use introspection/dbus* headers, so
  #        this dependency might be needed
  #libnmdbus_dep,
  libudev_dep,
  nm_core_dep,
  shared_n_acd_dep
]

if enable_concheck
  deps += libcurl_dep
endif

if enable_libaudit
  deps += libaudit_dep
endif

if enable_libpsl
  deps += libpsl_dep
endif

if enable_selinux
  deps += selinux_dep
endif

if enable_session_tracking
  deps += logind_dep
endif

libnetwork_manager = static_library(
  nm_name,
  sources: sources,
  dependencies: deps,
  c_args: cflags,
  link_with: [libnetwork_manager_base, libsystemd_nm]
)

ldflags = ['-rdynamic']

# FIXME: this doesn't work and it depends on libtool
'''
src/NetworkManager.ver: src/libNetworkManager.la $(core_plugins)
  $(AM_V_GEN) NM="$(NM)" "$(srcdir)/tools/create-exports-NetworkManager.sh" --called-from-make "$(srcdir)"

src_NetworkManager_LDFLAGS = \
  -rdynamic \
  -Wl,--version-script="src/NetworkManager.ver"

nm = find_program('gcc-nm', 'nm')
create_exports_networkmanager = join_paths(meson.source_root(), 'tools', 'create-exports-NetworkManager.sh')

symbol_map_name = 'NetworkManager.ver'

linker_script = custom_target(
  symbol_map_name,
  input: meson.source_root(),
  output: symbol_map_name,
  capture: true,
  #command: ['NM=' + nm.path(), create_exports_networkmanager, '--called-from-make', '@INPUT@']
  command: [create_exports_networkmanager, '--called-from-make', '@INPUT@']
)

ldflags += '-Wl,--version-script,@0@'.format(linker_script)
'''

network_manager = executable(
  nm_name,
  'main.c',
  dependencies: deps,
  c_args: cflags,
  link_with: libnetwork_manager,
  link_args: ldflags,
  install: true,
  install_dir: nm_sbindir
)
