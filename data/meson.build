data_conf = configuration_data()
data_conf.set('bindir', byx_bindir)
data_conf.set('libexecdir', byx_libexecdir)
data_conf.set('sbindir', byx_sbindir)
data_conf.set('sysconfdir', byx_sysconfdir)
data_conf.set('DISTRO_NETWORK_SERVICE', ((distro == 'redhat') ? 'network.service' : ''))

configure_file(
  input: 'server.conf.in',
  output: 'server.conf',
  install: true,
  install_dir: join_paths(byx_datadir, 'doc', byx_name, 'examples'),
  configuration: data_conf
)

if install_service
  configure_file(
    input: 'bombyx.service.in',
    output: 'bombyx.service',
    install: true,
    install_dir: systemd_system_unit_dir,
    configuration: data_conf
  )
endif

install_data(
  'org.fpemud.Bombyx.conf',
  install_dir: dbus_conf_dir
)
