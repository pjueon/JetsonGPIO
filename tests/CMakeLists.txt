set(_test_targets
    "test_all_apis"
    "test_event_module"
    )

foreach (test ${_test_targets})
  add_executable(${test} ${test}.cpp TestUtility.cpp)
  target_link_libraries(${test} PRIVATE JetsonGPIO)
endforeach ()


set(_unit_test_targets
    "test_dictionary_like"
    "test_lazy_string"
    "test_model_name"
    "test_format"
    "test_startswith"
    "test_lower"
    "test_none"
    "test_is_iterable"
    )


foreach (test ${_unit_test_targets})
  add_executable(${test} unit-tests/${test}.cpp TestUtility.cpp)
  add_test(NAME ${test} COMMAND $<TARGET_FILE:${test}>)
  target_link_libraries(${test} PRIVATE JetsonGPIO)
endforeach ()
