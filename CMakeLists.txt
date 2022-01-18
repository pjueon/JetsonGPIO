cmake_minimum_required(VERSION 3.2)
PROJECT(JetsonGPIO
        VERSION 1.2.0
        LANGUAGES C CXX)

include(GNUInstallDirs)
SET(CMAKE_CXX_STANDARD 14)

option(BUILD_EXAMPLES "Build examples" OFF)

SET (UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

ADD_DEFINITIONS(-Wall -O2 -fPIC -g)

# Sources
set(JetsonGPIO_LIB_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/JetsonGPIO.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PythonFunctions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gpio_pin_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gpio_event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DictionaryLike.cpp)

# Generate a *Config.h header in the build directory
configure_file(cmake/JetsonGPIOConfig.h.in JetsonGPIOConfig.h)

# Build a shared library
ADD_LIBRARY(JetsonGPIO SHARED ${JetsonGPIO_LIB_SRCS})
TARGET_LINK_LIBRARIES(JetsonGPIO Threads::Threads)
SET_TARGET_PROPERTIES(JetsonGPIO PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR})

target_include_directories(
  JetsonGPIO PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

add_library(JetsonGPIO::JetsonGPIO ALIAS JetsonGPIO)

# Install Library
INSTALL(TARGETS JetsonGPIO
        EXPORT JetsonGPIOTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install Public Headers
INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/JetsonGPIO.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install the auto-generated configuration header
install(FILES ${PROJECT_BINARY_DIR}/JetsonGPIOConfig.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# (Generate and) install the target import file
INSTALL(EXPORT JetsonGPIOTargets
        NAMESPACE JetsonGPIO::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JetsonGPIO)

# Generate the package version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/JetsonGPIOConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)

# Generate the package configuration file
configure_package_config_file(
  ${CMAKE_CURRENT_LIST_DIR}/cmake/JetsonGPIOConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/JetsonGPIOConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JetsonGPIO)

# Install the package version and configuration files
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/JetsonGPIOConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/JetsonGPIOConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JetsonGPIO)

# Install the README and LICENSE files
install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/README.md
        ${CMAKE_CURRENT_LIST_DIR}/LICENSE.txt
        DESTINATION ${CMAKE_INSTALL_DATADIR}/JetsonGPIO)


# Install Udev rules
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/99-gpio.rules DESTINATION ${UDEVRULES_INSTALL_DIR})

# Build Test
set(JetsonGPIO_test_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/test.cpp )

add_executable(jetsongpio_test ${JetsonGPIO_test_SRCS})
TARGET_LINK_LIBRARIES(jetsongpio_test JetsonGPIO Threads::Threads)

# Build Test for DictionaryLike
set(DictionaryLike_test_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/TestForDictionaryLike.cpp)

add_executable(dictionarylike_test ${DictionaryLike_test_SRCS})
TARGET_LINK_LIBRARIES(dictionarylike_test JetsonGPIO Threads::Threads)

# Build Test all APIs
set(test_all_apis_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/samples/test_all_apis.cpp )

add_executable(test_all_apis ${test_all_apis_SRCS})
TARGET_LINK_LIBRARIES(test_all_apis JetsonGPIO Threads::Threads)

# Build examples
if (BUILD_EXAMPLES)
add_subdirectory(samples)
endif()
