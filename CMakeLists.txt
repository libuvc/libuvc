cmake_minimum_required(VERSION 3.1)
project(libuvc
  VERSION 0.0.6
  LANGUAGES C
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Xcode and Visual Studio do not using CMAKE_BUILD_TYPE cache variable
# so to use Release build type with single configuration generators
if (NOT CMAKE_CONFIGURATION_TYPES)
  if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
  endif()
endif()

if(NOT CMAKE_BUILD_TARGET)
    message(STATUS "No target type selected, default to both shared and static library")
    set(CMAKE_BUILD_TARGET "Both" CACHE STRING "" FORCE)
endif()

option(BUILD_EXAMPLE "Build example program" OFF)
option(BUILD_TEST "Build test program" OFF)
option(ENABLE_UVC_DEBUGGING "Enable UVC debugging" ON)


set(libuvc_DESCRIPTION "A cross-platform library for USB video devices")
set(libuvc_URL "https://github.com/libuvc/libuvc")


find_package(LibUSB)


set(SOURCES 
  src/ctrl.c
  src/ctrl-gen.c
  src/device.c
  src/diag.c
  src/frame.c
  src/init.c
  src/stream.c
  src/misc.c
)

# JpegPkg name to differ from shipped with CMake
find_package(JpegPkg)
if(JPEG_FOUND)
  message(STATUS "Building libuvc with JPEG support.")
  set(LIBUVC_HAS_JPEG TRUE)
  list(APPEND SOURCES src/frame-mjpeg.c)
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
  message(FATAL_ERROR "Invalid build type ${CMAKE_BUILD_TARGET}" )
endif()

if(BUILD_UVC_SHARED)
  add_library(uvc SHARED ${SOURCES})
  set_target_properties(uvc PROPERTIES
    VERSION ${libuvc_VERSION}
    SOVERSION ${libuvc_VERSION_MAJOR}
    EXPORT_NAME UVCShared
  )
  list(APPEND UVC_TARGETS uvc)
  if(NOT LibUVC_STATIC)
    add_library(LibUVC::UVC ALIAS uvc)
  endif()
  add_library(LibUVC::UVCShared ALIAS uvc)
endif()

if(BUILD_UVC_STATIC)
  add_library(uvc_static STATIC ${SOURCES})
  set_target_properties(uvc_static PROPERTIES
    OUTPUT_NAME uvc
    EXPORT_NAME UVCStatic
  )
  list(APPEND UVC_TARGETS uvc_static)
  add_library(LibUVC::UVCStatic ALIAS uvc_static)
  if(LibUVC_STATIC)
    add_library(LibUVC::UVC ALIAS uvc_static)
  endif()
endif()

configure_file(include/libuvc/libuvc_config.h.in
  include/libuvc/libuvc_config.h
  @ONLY
)

foreach(target_name IN LISTS UVC_TARGETS)
  target_include_directories(${target_name}
    PUBLIC
      $<INSTALL_INTERFACE:include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include$<SEMICOLON>>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  )
  target_link_libraries(${target_name}
    PRIVATE LibUSB::LibUSB
  )
  if(JPEG_FOUND)
    target_link_libraries(${target_name}
      PRIVATE JPEG::JPEG
    )
  endif()
  set_target_properties(${target_name} PROPERTIES
      PUBLIC_HEADER "include/libuvc/libuvc.h;${libuvc_BINARY_DIR}/include/libuvc/libuvc_config.h"
  )
  if(ENABLE_UVC_DEBUGGING)
    target_compile_definitions(${target_name}
      PRIVATE
        UVC_DEBUGGING
    )
  endif()
endforeach()

if(BUILD_EXAMPLE)
  add_executable(example src/example.c)
  find_package(Threads)
  target_link_libraries(example
    PRIVATE
      LibUVC::UVC
      Threads::Threads
  )
endif()

if(BUILD_TEST)
  find_package(OpenCV
    REQUIRED COMPONENTS
      opencv_core
      opencv_highgui
  )

  add_executable(uvc_test src/test.c)
  target_link_libraries(uvc_test
    PRIVATE
      LibUVC::UVC
      opencv_core
      opencv_highgui
  )
endif()


include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/libuvc)

install(
  TARGETS ${UVC_TARGETS}
  EXPORT libuvcTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libuvc
)

install(EXPORT libuvcTargets
  FILE libuvcTargets.cmake
  NAMESPACE LibUVC::
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
)

install(FILES 
    cmake/FindLibUSB.cmake
    cmake/FindJpegPkg.cmake
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(libuvcConfigVersion.cmake
  COMPATIBILITY AnyNewerVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/libuvcConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
)

if(JPEG_FOUND)
  set(PKGCONFIG_JPEG_LDFLAG "-ljpeg")
endif()

configure_file(libuvc.pc.in
  ${PROJECT_BINARY_DIR}/libuvc.pc
  @ONLY
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/libuvc.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

install(FILES libuvcConfig.cmake
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
)


