# a list of dicts describing samples
samples = [
  {
    'description': 'simple out example',
    'filename': 'simple_out',
    'sources': ['simple_out.cpp'],
  },
  {
    'description': 'simple pwm example',
    'filename': 'simple_pwm',
    'sources': ['simple_pwm.cpp'],
  },
]

# build samples
foreach sample: samples
  exe = executable(sample['filename'], sample['sources'],
    dependencies: JetsonGPIO_deps,
    link_with: libJetsonGPIO,
    cpp_args: JetsonGPIO_cpp_args,
    include_directories: JetsonGPIO_incdirs,
  )
endforeach