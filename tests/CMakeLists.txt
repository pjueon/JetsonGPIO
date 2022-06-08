set(_test_targets
    "test_all_apis"
    "test_event_module"
    )

foreach (test ${_test_targets})
  add_executable(${test} ${test}.cpp)
  target_link_libraries(${test} PRIVATE JetsonGPIO)
endforeach ()


set(_unit_test_targets
    "test_dictionary_like"
    "test_lazy_string"
    )

foreach (test ${_unit_test_targets})
  add_executable(${test} unit-tests/${test}.cpp)
  target_link_libraries(${test} PRIVATE JetsonGPIO)
endforeach ()