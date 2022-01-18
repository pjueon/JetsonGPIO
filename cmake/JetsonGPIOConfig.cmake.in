include(CMakeFindDependencyMacro)
find_dependency(Threads)

get_filename_component(JetsonGPIO_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
if (NOT TARGET JetsonGPIO::JetsonGPIO)
  include("${JetsonGPIO_CMAKE_DIR}/JetsonGPIOTargets.cmake")
endif ()

set(JetsonGPIO_LIBRARIES JetsonGPIO::JetsonGPIO)
