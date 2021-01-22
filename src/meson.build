JetsonGPIO_src = [
  'gpio_pin_data.cpp',    # tegra pin data
  'PythonFunctions.cpp',  # python like functions
  'JetsonGPIO.cpp',       # everything else
]

JetsonGPIO_deps = []

JetsonGPIO_cpp_args = []

libJetsonGPIO = library(
  # args (lib name, source):
  meson.project_name() + '-' + proj_ver_short, JetsonGPIO_src,
  # kwargs:
  dependencies: JetsonGPIO_deps,
  cpp_args: JetsonGPIO_cpp_args,
  include_directories: JetsonGPIO_incdirs,
  install: true,
)

# this can be used in subprojects by getting this variable
JetsonGPIO_dep = declare_dependency(
  link_with: libJetsonGPIO,
  include_directories: JetsonGPIO_incdirs,
  dependencies: JetsonGPIO_deps,
)

# generate and install a .pc so other build systems (eg. cmake, autotools) can
# use the generated library easily.
pkg = import('pkgconfig')
JetsonGPIO_pc = pkg.generate(libJetsonGPIO,
  description: project_description,
  url: project_url,
  install_dir: get_option('datadir') / 'pkgconfig',
  subdirs: meson.project_name(),
)