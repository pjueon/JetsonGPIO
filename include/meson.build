# configuration
JetsonGPIO_config_h = configure_file(
  input: 'JetsonGPIO_config.h.in',
  output: 'JetsonGPIO_config.h',
  configuration: {
    # Version info:
    'version': meson.project_version(),
    # 'version_short': proj_ver_short,
    # 'version_major': proj_ver_major.to_int(),
    # 'version_minor': proj_ver_minor.to_int(),
    # 'version_patch': proj_ver_patch.to_int(),
  },
)

# this variable gets used to build the library in src/
JetsonGPIO_incdirs = [
    include_directories('.'),
    include_directories('./private'),
]

# public headers
public_headers = [
  JetsonGPIO_config_h,
  'JetsonGPIO.h',
]

# install public headers to an include subdir named after the project
install_headers(
  public_headers,
  subdir: meson.project_name(),
)