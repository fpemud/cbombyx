version_conf = configuration_data()
version_conf.set('NM_MAJOR_VERSION', byx_major_version)
version_conf.set('NM_MINOR_VERSION', byx_minor_version)
version_conf.set('NM_MICRO_VERSION', byx_micro_version)

version_header = configure_file(
    input: 'byx-version-macros.h.in',
    output: 'byx-version-macros.h',
    configuration: version_conf,
)
