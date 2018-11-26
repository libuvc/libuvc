cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0042 NEW)
project(libuvc)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

if (NOT CMAKE_BUILD_TARGET)
  message(STATUS "No target type selected, default to shared  library")
  set(CMAKE_BUILD_TARGET "Shared" CACHE STRING "" FORCE)
endif()

set(libuvc_VERSION_MAJOR 0)
set(libuvc_VERSION_MINOR 0)
set(libuvc_VERSION_PATCH 9)
set(libuvc_VERSION ${libuvc_VERSION_MAJOR}.${libuvc_VERSION_MINOR}.${libuvc_VERSION_PATCH})

set(libuvc_DESCRIPTION "A cross-platform library for USB video devices")
set(libuvc_URL "https://github.com/ktossell/libuvc")

if(WIN32)

  find_library(LIBUSB_LIBRARIES usb-1.0
  PATHS /opt/local/lib)

  find_path(LIBUSB_INCLUDE_DIR libusb-1.0/libusb.h
  PATHS /opt/local/include)
else()
  find_package(PkgConfig)
  pkg_check_modules(LIBUSB libusb-1.0)
endif()


if(WIN32)
  find_library(PTHREAD_LIBRARIES pthreadVC2
        PATHS D:/install/pthreads-w32-2-9-1-release/lib/x64)
  find_path(PTHREAD_INCLUDE_DIR pthread.h
        PATHS D:/install/pthreads-w32-2-9-1-release/include)
  SET(EXTRA_DEFS "-D_TIMESPEC_DEFINED")
  add_definitions(${EXTRA_DEFS})
else()
  include(GNUInstallDirs)
  SET(CMAKE_C_FLAGS_DEBUG "-g  -DUVC_DEBUGGING")
endif()

SET(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_PREFIX}/lib/cmake/libuvc" CACHE PATH
  "Installation directory for CMake files")

SET(SOURCES src/ctrl.c src/ctrl-gen.c src/device.c src/diag.c
           src/frame.c src/init.c src/stream.c
           src/misc.c)

include_directories(
  ${libuvc_SOURCE_DIR}/include
  ${libuvc_BINARY_DIR}/include
  ${LIBUSB_INCLUDE_DIR}
  ${PTHREAD_INCLUDE_DIR}
)

if(${CMAKE_BUILD_TARGET} MATCHES "Shared")
  set(BUILD_UVC_SHARED TRUE)
elseif(${CMAKE_BUILD_TARGET} MATCHES "Static")
  set(BUILD_UVC_STATIC TRUE)
elseif(${CMAKE_BUILD_TARGET} MATCHES "Both")
  set(BUILD_UVC_SHARED TRUE)
  set(BUILD_UVC_STATIC TRUE)
else()
  message( FATAL_ERROR "Invalid build type ${CMAKE_BUILD_TARGET}" )
endif()

if(BUILD_UVC_SHARED)
  add_library(uvc SHARED ${SOURCES})
  list(APPEND UVC_TARGETS uvc)
endif()

if(BUILD_UVC_STATIC)
  add_library(uvc_static STATIC ${SOURCES})
  set_target_properties(uvc_static PROPERTIES OUTPUT_NAME uvc)
  list(APPEND UVC_TARGETS uvc_static)
endif()

configure_file(include/libuvc/libuvc_config.h.in
  ${PROJECT_BINARY_DIR}/include/libuvc/libuvc_config.h @ONLY)

foreach(target_name ${UVC_TARGETS})
  set_target_properties(${target_name} PROPERTIES
    PUBLIC_HEADER "include/libuvc/libuvc.h;${libuvc_BINARY_DIR}/include/libuvc/libuvc_config.h"  WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
  set_target_properties(${target_name} PROPERTIES
                        VERSION ${libuvc_VERSION}
                        SOVERSION ${libuvc_VERSION_MAJOR})
endforeach()

if(BUILD_UVC_SHARED)
  target_link_libraries(uvc ${LIBUSB_LIBRARIES} ${PTHREAD_LIBRARIES})

  #add_executable(test src/test.c)
  #target_link_libraries(test uvc ${LIBUSB_LIBRARIES} opencv_highgui
  #  opencv_core)
endif()

install(TARGETS ${UVC_TARGETS}
  EXPORT libuvcTargets
  LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libuvc"
)

export(TARGETS ${UVC_TARGETS}
  FILE "${PROJECT_BINARY_DIR}/libuvcTargets.cmake")
export(PACKAGE libuvc)

set(CONF_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
set(CONF_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib")
set(CONF_LIBRARY "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}uvc${CMAKE_SHARED_LIBRARY_SUFFIX}")

configure_file(libuvcConfig.cmake.in ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libuvcConfig.cmake)

configure_file(libuvcConfigVersion.cmake.in ${PROJECT_BINARY_DIR}/libuvcConfigVersion.cmake @ONLY)

configure_file(libuvc.pc.in ${PROJECT_BINARY_DIR}/libuvc.pc @ONLY)

install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libuvcConfig.cmake"
  "${PROJECT_BINARY_DIR}/libuvcConfigVersion.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}")

install(EXPORT libuvcTargets
  DESTINATION "${INSTALL_CMAKE_DIR}")

install(FILES
  "${PROJECT_BINARY_DIR}/libuvc.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
