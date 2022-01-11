PROJECT(JetsonGPIO C CXX)
cmake_minimum_required(VERSION 3.2)

include(GNUInstallDirs)
SET(CMAKE_CXX_STANDARD 14)

SET(LIBRARY_VERSION "1.0.3")
SET(LIBRARY_SOVERSION "1")

SET (UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

ADD_DEFINITIONS(-Wall -O2 -fPIC -g)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Sources
set(JetsonGPIO_LIB_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/JetsonGPIO.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PythonFunctions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gpio_pin_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gpio_event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DictionaryLike.cpp)

# Build a shared library
ADD_LIBRARY(JetsonGPIO SHARED ${JetsonGPIO_LIB_SRCS})
TARGET_LINK_LIBRARIES(JetsonGPIO Threads::Threads)
SET_TARGET_PROPERTIES(JetsonGPIO PROPERTIES VERSION ${LIBRARY_VERSION} SOVERSION ${LIBRARY_SOVERSION})

# Install Library
INSTALL(TARGETS JetsonGPIO LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install Public Headers
INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/JetsonGPIO.h
    DESTINATION include/JetsonGPIO)

# Install Udev rules
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/99-gpio.rules DESTINATION ${UDEVRULES_INSTALL_DIR})

# Build Test
set(JetsonGPIO_test_SRCS
   ${JetsonGPIO_LIB_SRCS}
   ${CMAKE_CURRENT_SOURCE_DIR}/src/test.cpp )

add_executable(jetsongpio_test ${JetsonGPIO_test_SRCS})
TARGET_LINK_LIBRARIES(jetsongpio_test Threads::Threads)

# Build Test for DictionaryLike
set(DictionaryLike_test_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/DictionaryLike.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/TestForDictionaryLike.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/PythonFunctions.cpp)

add_executable(dictionarylike_test ${DictionaryLike_test_SRCS})
# Build Test all APIs
set(test_all_apis_SRCS
   ${JetsonGPIO_LIB_SRCS}
   ${CMAKE_CURRENT_SOURCE_DIR}/samples/test_all_apis.cpp )

add_executable(test_all_apis ${test_all_apis_SRCS})
TARGET_LINK_LIBRARIES(test_all_apis Threads::Threads)
