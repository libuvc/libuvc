cmake_minimum_required(VERSION 3.23)
cmake_policy(SET CMP0042 NEW)
project(libuvc
  VERSION 0.1.0
  LANGUAGES C
  DESCRIPTION "A cross-platform library for USB video devices"
  HOMEPAGE_URL "https://github.com/pupil-labs/libuvc"
)
cmake_policy(SET CMP0054 NEW)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

if(WIN32)
  set(LIBUSB_WIN_INCLUDE_DIR "NOTSET" CACHE PATH "Path to the libusb include dir")
  set(LIBUSB_WIN_IMPORT_LIB_PATH "NOTSET" CACHE FILEPATH "Path to the libusb import library")

  # FindThreads will not find pthread.h with MSVC so we use pthreads-win
  set(PTHREADS_WIN_INCLUDE_DIR "NOTSET" CACHE PATH "Path to the pthreads-win include dir")
  set(PTHREADS_WIN_IMPORT_LIB_PATH "NOTSET" CACHE FILEPATH "Path to the pthreads-win import library")

  list(APPEND required_variables
    PTHREADS_WIN_INCLUDE_DIR
    PTHREADS_WIN_IMPORT_LIB_PATH
    LIBUSB_WIN_INCLUDE_DIR
    LIBUSB_WIN_IMPORT_LIB_PATH
  )
  message(STATUS "Validating config:")

  foreach(VAR IN LISTS required_variables)
    if(${VAR} STREQUAL "NOTSET")
      message(FATAL_ERROR "Required variable ${VAR} is not set!")
    endif()
  endforeach()

  foreach(VAR IN LISTS required_variables)
    message(STATUS "  ${VAR}: ${${VAR}}")
  endforeach()

  # winmm is necessary for __imp_timeGetTime
  add_definitions("-D_TIMESPEC_DEFINED")
else()
  find_package(PkgConfig)
  pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
  message(STATUS "    " ${LIBUSB_LIBRARY_DIRS})
  SET(CMAKE_C_FLAGS_DEBUG "-g  -DUVC_DEBUGGING")
endif()

include(GNUInstallDirs)

# "Installation directory for CMake files")
add_library(uvc SHARED)
add_library(PLLibUVC::uvc ALIAS uvc)

target_sources(uvc PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/ctrl.c>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/ctrl-gen.c>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/device.c>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/diag.c>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/frame.c>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/init.c>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/stream.c>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/misc.c>
)

target_include_directories(
  uvc
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

  # Unix vars
  ${LIBUSB_INCLUDE_DIRS}
  ${PTHREAD_INCLUDE_DIRS}

  # Windows vars
  ${PTHREADS_WIN_INCLUDE_DIR}
  ${LIBUSB_WIN_INCLUDE_DIR}
)

target_sources(uvc
  PUBLIC
  FILE_SET HEADERS
  BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
  FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/libuvc/libuvc.h
)

target_link_directories(
  uvc
  PUBLIC
  ${LIBUSB_LIBRARY_DIRS}
  ${PTHREAD_LIBRARY_DIRS}
)

set_target_properties(
  uvc PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS TRUE
  VERSION ${libuvc_VERSION}
  SOVERSION ${libuvc_VERSION}
)

if(UNIX AND NOT APPLE)
  message(STATUS "Adding extra -pthread c flag")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
endif()

target_link_libraries(
  uvc
  PUBLIC

  # Unix vars
  ${LIBUSB_LIBRARIES}
  ${PTHREAD_LIBRARIES}

  # Windows vars
  ${PTHREADS_WIN_IMPORT_LIB_PATH}
  ${LIBUSB_WIN_IMPORT_LIB_PATH}
)

install(TARGETS uvc
  EXPORT libuvcTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILE_SET HEADERS
)

# export(PACKAGE libuvc)
install(EXPORT libuvcTargets
  FILE libuvcTargets.cmake
  NAMESPACE PLLibUVC::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libuvc
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/libuvcConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/libuvcConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libuvc
)

export(EXPORT libuvcTargets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/libuvcTargets.cmake
  NAMESPACE PLLibUVC::
)

include(CMakePackageConfigHelpers)

set(LIBRARY_DIRS ${CMAKE_INSTALL_LIBDIR})

# generate the config file that is includes the exports
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/libuvcConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libuvc
  PATH_VARS LIBRARY_DIRS
)

write_basic_package_version_file(
  libuvcConfigVersion.cmake
  VERSION ${libuvc_VERSION}
  COMPATIBILITY SameMajorVersion
)

enable_testing()
add_subdirectory(tests)
