# - Try to find JetsonGPIO Library
# Once done this will define
#
#  JetsonGPIO_FOUND - system has JetsonGPIO
#  JetsonGPIO_INCLUDE_DIR - the JetsonGPIO include directory
#  JetsonGPIO_LIBRARIES - Link these to use JetsonGPIO

# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (JetsonGPIO_INCLUDE_DIR AND JetsonGPIO_LIBRARIES)

  # in cache already
  set(JetsonGPIO_FOUND TRUE)
  message(STATUS "Found libJetsonGPIO: ${JetsonGPIO_LIBRARIES}")

else (JetsonGPIO_INCLUDE_DIR AND JetsonGPIO_LIBRARIES)

  find_path(JetsonGPIO_INCLUDE_DIR JetsonGPIO.h
    PATH_SUFFIXES JetsonGPIO
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(JetsonGPIO_LIBRARIES NAMES JetsonGPIO
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )    

  if(JetsonGPIO_INCLUDE_DIR AND JetsonGPIO_LIBRARIES)
    set(JetsonGPIO_FOUND TRUE)
  else (JetsonGPIO_INCLUDE_DIR AND JetsonGPIO_LIBRARIES)
    set(JetsonGPIO_FOUND FALSE)
  endif(JetsonGPIO_INCLUDE_DIR AND JetsonGPIO_LIBRARIES)


  if (JetsonGPIO_FOUND)
    if (NOT JetsonGPIO_FIND_QUIETLY)
      message(STATUS "Found JetsonGPIO: ${JetsonGPIO_LIBRARIES}")
    endif (NOT JetsonGPIO_FIND_QUIETLY)
  else (JetsonGPIO_FOUND)
    if (JetsonGPIO_FIND_REQUIRED)
      message(FATAL_ERROR "JetsonGPIO not found. Please install JetsonGPIO https://github.com/pjueon/JetsonGPIO")
    endif (JetsonGPIO_FIND_REQUIRED)
  endif (JetsonGPIO_FOUND)

  mark_as_advanced(JetsonGPIO_INCLUDE_DIR JetsonGPIO_LIBRARIES)

endif (JetsonGPIO_INCLUDE_DIR AND JetsonGPIO_LIBRARIES)
