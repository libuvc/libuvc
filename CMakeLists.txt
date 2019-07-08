cmake_minimum_required(VERSION 2.8)
project(libuvc)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

if (NOT CMAKE_BUILD_TARGET)
  message(STATUS "No target type selected, default to both shared and static library")
  set(CMAKE_BUILD_TARGET "Both" CACHE STRING "" FORCE)
endif()

option(BUILD_EXAMPLE "Build example program" OFF)
option(BUILD_TEST "Build test program" OFF)

set(libuvc_VERSION_MAJOR 0)
set(libuvc_VERSION_MINOR 0)
set(libuvc_VERSION_PATCH 6)
set(libuvc_VERSION ${libuvc_VERSION_MAJOR}.${libuvc_VERSION_MINOR}.${libuvc_VERSION_PATCH})

set(libuvc_DESCRIPTION "A cross-platform library for USB video devices")
set(libuvc_URL "https://github.com/ktossell/libuvc")

find_package(PkgConfig)
pkg_check_modules(LIBUSB libusb-1.0)

# Try to find JPEG using a module or pkg-config. If that doesn't work, search for the header.
find_package(jpeg QUIET)
if(JPEG_FOUND)
  set(JPEG_LINK_FLAGS ${JPEG_LIBRARIES})
else()
  pkg_check_modules(JPEG QUIET libjpeg)
  if(JPEG_FOUND)
      set(JPEG_INCLUDE_DIR ${JPEG_INCLUDE_DIRS})
      set(JPEG_LINK_FLAGS ${JPEG_LDFLAGS})
  else()
    find_path(JPEG_INCLUDE_DIR jpeglib.h)
    if(JPEG_INCLUDE_DIR)
      set(JPEG_FOUND ON)
      set(JPEG_LINK_FLAGS -ljpeg)
    endif()
  endif()
endif()

include(GNUInstallDirs)

SET(CMAKE_C_FLAGS_DEBUG "-g -DUVC_DEBUGGING")

SET(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_PREFIX}/lib/cmake/libuvc" CACHE PATH
	"Installation directory for CMake files")

SET(SOURCES src/ctrl.c src/ctrl-gen.c src/device.c src/diag.c
           src/frame.c src/init.c src/stream.c
           src/misc.c)

include_directories(
  ${libuvc_SOURCE_DIR}/include
  ${libuvc_BINARY_DIR}/include
  ${LIBUSB_INCLUDE_DIRS}
)

if(JPEG_FOUND)
  message(STATUS "Building libuvc with JPEG support.")
  include_directories(${JPEG_INCLUDE_DIR})
  SET(LIBUVC_HAS_JPEG TRUE)
  SET(SOURCES ${SOURCES} src/frame-mjpeg.c)
else()
  message(WARNING "JPEG not found. libuvc will not support JPEG decoding.")
endif()

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
  set_property(TARGET uvc PROPERTY VERSION ${libuvc_VERSION} )
  set_property(TARGET uvc PROPERTY SOVERSION ${libuvc_VERSION_MAJOR} )
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
    PUBLIC_HEADER "include/libuvc/libuvc.h;${libuvc_BINARY_DIR}/include/libuvc/libuvc_config.h" )
endforeach()

if(BUILD_UVC_SHARED)
  if(JPEG_FOUND)
    target_link_libraries (uvc ${JPEG_LINK_FLAGS})
  endif(JPEG_FOUND)

  target_link_libraries(uvc ${LIBUSB_LIBRARIES})

  #add_executable(test src/test.c)
  #target_link_libraries(test uvc ${LIBUSB_LIBRARIES} opencv_highgui
  #  opencv_core)
endif()

if(BUILD_EXAMPLE)
  add_executable(example src/example.c)
  target_link_libraries(example uvc pthread)
endif()

if(BUILD_TEST)

    # Try to find OpenCV using a module or pkg-config. If that doesn't work, search for the header.
  find_package(opencv QUIET)
  if(OPENCV_FOUND)
    set(OPENCV_LINK_FLAGS ${OPENCV_LIBRARIES})
  else()
    pkg_check_modules(OPENCV QUIET opencv)
    if(OPENCV_FOUND)
        set(OPENCV_INCLUDE_DIR ${OPENCV_INCLUDE_DIRS})
        set(OPENCV_LINK_FLAGS ${OPENCV_LDFLAGS})
    endif()
  endif()

  add_executable(uvc_test src/test.c)
  target_link_libraries(uvc_test uvc ${OPENCV_LDFLAGS})
endif()

install(TARGETS ${UVC_TARGETS}
    EXPORT libuvcTargets
  LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_LIBRARY_ARCHITECTURE}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_LIBRARY_ARCHITECTURE}"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libuvc"
)

export(TARGETS ${UVC_TARGETS}
  FILE "${PROJECT_BINARY_DIR}/libuvcTargets.cmake")
export(PACKAGE libuvc)

set(CONF_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
set(CONF_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_LIBRARY_ARCHITECTURE}")
set(CONF_LIBRARY "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_LIBRARY_ARCHITECTURE}/${CMAKE_SHARED_LIBRARY_PREFIX}uvc${CMAKE_SHARED_LIBRARY_SUFFIX}")

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
