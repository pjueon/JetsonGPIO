# this meson build is to test we didn't break gnu-make

# autodetect number of cpus
nproc = find_program('nproc')
if nproc.found()
  cpus = run_command(nproc).stdout().strip().to_int()
else
  # we'll just guess
  cpus = 4
endif

# make args for all targets (except clean)
make_args = ['-j' + cpus.to_string()]

# make targets
tests = [
  {
    'description': 'test make all',
    'args': ['all'],
    'suites': ['gnu_make'],
  },
  {
    'description': 'test make Test',
    'args': ['Test'],
    'suites': ['gnu_make'],
  },
]

gnu_make = find_program('make')

if gnu_make.found()
  test('preparatory make clean', gnu_make,
    protocol: 'exitcode',
    args: ['clean'],
    suite: t['suites'],
    workdir: meson.current_source_dir(),
    is_parallel: false,
  )
  foreach t: tests
    test(t['description'], gnu_make,
      protocol: 'exitcode',
      args: make_args + t['args'],
      suite: t['suites'],
      workdir: meson.current_source_dir(),
      is_parallel: false,
    )
    # this is necessary to make sure the makefile target dependencies work
    test('post-test make clean', gnu_make,
      protocol: 'exitcode',
      args: ['clean'],
      suite: t['suites'],
      workdir: meson.current_source_dir(),
      is_parallel: false,
    )
  endforeach
  test('final make clean', gnu_make,
    protocol: 'exitcode',
    args: ['clean'],
    suite: t['suites'],
    workdir: meson.current_source_dir(),
    is_parallel: false,
  )
else
  message('gnu-make not found. skipping make tests')
endif