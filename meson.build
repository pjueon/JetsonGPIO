# setup the project's global options
project('JetsonGPIO', ['cpp'],
  version: run_command(
    'head', '-n', '1', files('VERSION'),
  ).stdout().strip(),
  meson_version: '>= 0.56.0',
  license: 'MIT',
  default_options: [
    'cpp_std=c++17',
    # 'warning_level=2',  # -Wall, Wextra
    # 'werror=true',
  ],
)

# the c++ compiler object
cc = meson.get_compiler('cpp')

# split the project version
proj_ver = meson.project_version().split('.')
# version minus patch revision
proj_ver_short = proj_ver[0] + '.' + proj_ver[1]
proj_ver_major = proj_ver[0]
proj_ver_minor = proj_ver[1]
proj_ver_patch = proj_ver[2]
# all of this is used by configure_file on config.h.in
project_description = 'JetsonGPIO c++ port'
binary_package = 'source'
origin = 'github'
# change this if you fork this
project_url = 'https://github.com/mdegans/JetsonGPIO'

# extra global cflags:
add_project_arguments(
  '-DUSING_MESON',
  language : 'cpp'
)

subdir('include')
subdir('src')
# subdir('samples')  # currently do not build
subdir('test')
subdir('build')
