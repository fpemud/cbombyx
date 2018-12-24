ifaces = [
  'org.fpemud.Bombyx',
  'org.fpemud.Bombyx.Connection',
]

ifaces_xmls = []
sources = []

# FIXME?: DBUS_INTERFACE_DOCS/docbook files are generated in
# "meson.current_build_dir()" instead of "meson.source_root()/docs/api"
foreach iface: ifaces
  iface_xml = iface + '.xml'
  ifaces_xmls += files(iface_xml)

  sources += gnome.gdbus_codegen(
    iface,
    iface_xml,
    interface_prefix: 'org.fpemud.Bombyx',
    namespace: 'NMDBus',
    docbook: 'dbus'
  )

  # FIXME: gnome.gdbus_codegen should return the docbook as a target result
  content_files += join_paths(meson.current_build_dir(), 'dbus-' + iface_xml)
endforeach

install_data(
  ifaces_xmls,
  install_dir: dbus_ifaces_dir
)
