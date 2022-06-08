set(_example_targets
    "button_event"
    "button_interrupt"
    "simple_out"
    "simple_pwm"
    "jetson_model"
    )

foreach (example ${_example_targets})
  add_executable(${example} ${example}.cpp)
  target_link_libraries(${example} PRIVATE JetsonGPIO)
endforeach ()

add_custom_target(examples)
add_dependencies(examples ${_example_targets})
