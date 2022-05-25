set(_test_targets
    "test_all_apis"
    "test_dictionary_like"
    "test_event_module"
    "test_lazy_string"
    )

foreach (test ${_test_targets})
  add_executable(${test} ${test}.cpp)
  target_link_libraries(${test} PRIVATE JetsonGPIO)
endforeach ()