data_conf = configuration_data()
data_conf.set('bindir', nm_bindir)
data_conf.set('libexecdir', nm_libexecdir)
data_conf.set('sbindir', nm_sbindir)
data_conf.set('sysconfdir', nm_sysconfdir)
data_conf.set('DISTRO_NETWORK_SERVICE', ((distro == 'redhat') ? 'network.service' : ''))

server = 'server.conf'

configure_file(
  input: server + '.in',
  output: server,
  install: true,
  install_dir: join_paths(nm_datadir, 'doc', nm_name, 'examples'),
  configuration: data_conf
)

if install_systemd_unit_dir
  services = [
    ['NetworkManager.service', systemd_system_unit_dir],
    ['org.freedesktop.NetworkManager.service', dbus_sys_dir]
  ]

  if have_systemd_200
    services += [['NetworkManager-wait-online.service', systemd_system_unit_dir]]
  else
    services += [['NetworkManager-wait-online-systemd-pre200.service', systemd_system_unit_dir]]
  endif

  foreach service: services
    configure_file(
      input: service[0] + '.in',
      output: service[0],
      install: true,
      install_dir: service[1],
      configuration: data_conf
    )
  endforeach

endif
