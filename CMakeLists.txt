cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0042 NEW)
project(libuvc
  VERSION 0.1.0
  LANGUAGES C
  DESCRIPTION "A cross-platform library for USB video devices"
  HOMEPAGE_URL "https://github.com/pupil-labs/libuvc"
)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

if(WIN32)
  find_library(LIBUSB_LIBRARIES usb-1.0
    PATHS /opt/local/lib)

  find_path(LIBUSB_INCLUDE_DIRS libusb-1.0/libusb.h
    PATHS /opt/local/include)
else()
  find_package(PkgConfig)
  pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
endif()

if(WIN32)
  find_library(PTHREAD_LIBRARIES pthreadVC2
    PATHS D:/install/pthreads-w32-2-9-1-release/lib/x64)
  find_path(PTHREAD_INCLUDE_DIRS pthread.h
    PATHS D:/install/pthreads-w32-2-9-1-release/include)
  SET(EXTRA_DEFS "-D_TIMESPEC_DEFINED")
  add_definitions(${EXTRA_DEFS})
else()
  include(GNUInstallDirs)
  SET(CMAKE_C_FLAGS_DEBUG "-g  -DUVC_DEBUGGING")
endif()

# SET(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_PREFIX}/lib/cmake/libuvc" CACHE PATH
set(CMAKE_INSTALL_NAME_DIR $<INSTALL_PREFIX>/${CMAKE_INSTALL_LIBDIR})

# "Installation directory for CMake files")
add_library(uvc SHARED)
add_library(PLLibUVC::uvc ALIAS uvc)

target_sources(uvc PUBLIC
  src/ctrl.c
  src/ctrl-gen.c
  src/device.c
  src/diag.c
  src/frame.c
  src/init.c
  src/stream.c
  src/misc.c
)

target_include_directories(
  uvc
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
  ${LIBUSB_INCLUDE_DIRS}
  ${PTHREAD_INCLUDE_DIRS}
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
  PRIVATE
  ${LIBUSB_LIBRARY_DIRS}
  ${PTHREAD_LIBRARY_DIRS}
)

set_target_properties(
  uvc PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS TRUE
  VERSION ${libuvc_VERSION}
  SOVERSION ${libuvc_VERSION}
)
target_link_libraries(uvc PRIVATE ${LIBUSB_LIBRARIES} ${PTHREAD_LIBRARIES})

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
  DESTINATION lib/cmake/libuvc
)

export(EXPORT libuvcTargets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/libuvcTargets.cmake
  NAMESPACE PLLibUVC::
)

include(CMakePackageConfigHelpers)

# generate the config file that is includes the exports
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/libuvcConfig.cmake"
  INSTALL_DESTINATION lib/cmake/libuvc
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  libuvcConfigVersion.cmake
  VERSION ${libuvc_VERSION}
  COMPATIBILITY SameMajorVersion
)
