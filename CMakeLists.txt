cmake_minimum_required(VERSION 3.2)
enable_testing()

project(JetsonGPIO
        VERSION 1.2.5
        LANGUAGES C CXX)

include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 14)

option(BUILD_EXAMPLES "Build examples" ON)
option(JETSON_GPIO_POST_INSTALL "Create gpio group and reload udev rules" ON)

set(UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

add_definitions(-Wall -O2 -fPIC -g)

# Sources
set(JetsonGPIO_LIB_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/JetsonGPIO.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainModule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PWM.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PythonFunctions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ExceptionHandling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GPIOPinData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GPIOEvent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Callback.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DictionaryLike.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ModelUtility.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WaitResult.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LazyString.cpp
    )

# Generate a *Config.h header in the build directory
configure_file(cmake/JetsonGPIOConfig.h.in JetsonGPIOConfig.h)

# Build a shared library
add_library(JetsonGPIO SHARED ${JetsonGPIO_LIB_SRCS})
target_link_libraries(JetsonGPIO Threads::Threads)
set_target_properties(JetsonGPIO PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR})

target_include_directories(JetsonGPIO PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

add_library(JetsonGPIO::JetsonGPIO ALIAS JetsonGPIO)

# Install Library
install(TARGETS JetsonGPIO
        EXPORT JetsonGPIOTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install Public Headers
install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/JetsonGPIO.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

file(GLOB JetsonGPIO_Public_Headers ${CMAKE_CURRENT_SOURCE_DIR}/include/JetsonGPIO/*.h)
install(FILES ${JetsonGPIO_Public_Headers} DESTINATION include/JetsonGPIO)

# Install the auto-generated configuration header
install(FILES ${PROJECT_BINARY_DIR}/JetsonGPIOConfig.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# (Generate and) install the target import file
install(EXPORT JetsonGPIOTargets
        NAMESPACE JetsonGPIO::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JetsonGPIO)

# Generate the package version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/JetsonGPIOConfigVersion.cmake
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY AnyNewerVersion)

# Generate the package configuration file
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/JetsonGPIOConfig.cmake.in
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

# Run post installation script
if (JETSON_GPIO_POST_INSTALL)
install (CODE "execute_process(COMMAND bash ./scripts/post_install.sh WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
endif()

# Build test codes 
add_subdirectory(tests)

# Build examples
if (BUILD_EXAMPLES)
add_subdirectory(samples)
endif()
