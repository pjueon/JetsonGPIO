# this meson.build is for integration tests

# a list of dicts describing tests
tests = [
  {
    'description': 'test pin 18 high and low',
    'filename': 'test_pin18_high_low',
    'sources': ['test_pin18_high_low.cpp'],
    'suites': ['interactive'],  # "interactive" is not run automatically
    'parallel': false,  # whether the test can run in parallel with others
  },
]

# build and run tests
foreach t: tests
  exe = executable(t['filename'], t['sources'],
    dependencies: JetsonGPIO_deps,
    link_with: libJetsonGPIO,
    cpp_args: JetsonGPIO_cpp_args,
    include_directories: JetsonGPIO_incdirs,
  )
  # skip running test if it's an interactive one
  if 'interactive' in t['suites']
    continue
  endif
  # run the rest of the tests
  test(t['description'], exe,
    protocol: 'exitcode',
    is_parallel: t['parallel'],
    suite: t['suites'],
  )
endforeach