cmake_minimum_required(VERSION 2.8)
project(libuvc)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

if (NOT CMAKE_BUILD_TARGET)
  message(STATUS "No target type selected, default to shared library")
  set(CMAKE_BUILD_TARGET "Shared" CACHE STRING "" FORCE)
endif()

set(libuvc_VERSION_MAJOR 0)
set(libuvc_VERSION_MINOR 0)
set(libuvc_VERSION_PATCH 5)
set(libuvc_VERSION ${libuvc_VERSION_MAJOR}.${libuvc_VERSION_MINOR}.${libuvc_VERSION_PATCH})

find_package(PkgConfig)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

# Try to find JPEG using a module or pkg-config. If that doesn't work, search for the header.
find_package(jpeg QUIET)
if(NOT JPEG_FOUND)
  find_path(JPEG_INCLUDE_DIR jpeglib.h)
  if(JPEG_INCLUDE_DIR)
    set(JPEG_FOUND ON)
    set(JPEG_LIBRARIES -ljpeg)
  endif()
endif()

SET(CMAKE_C_FLAGS_DEBUG "-g -DUVC_DEBUGGING")

# Offer the user the choice of overriding the installation directories
SET(INSTALL_INCLUDE_DIR include CACHE PATH
  "Installation directory for header files")
SET(INSTALL_CMAKE_DIR lib/cmake/libuvc CACHE PATH
  "Installation directory for CMake files")

# Create absolute versions of install paths (needed later on)
foreach(p INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  set(ABS_${var} ${${var}})  # i.e. ABS_INSTALL_INCLUDE_DIR
  if(NOT IS_ABSOLUTE "${${var}}")
    set(ABS_${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

# Set up include-directories
include_directories(
  ${libuvc_SOURCE_DIR}/include
  ${libuvc_BINARY_DIR}/include
  ${LIBUSB_INCLUDE_DIRS}
)

# Configure the uvc library
SET(SOURCES src/ctrl.c src/ctrl-gen.c src/device.c src/diag.c
           src/frame.c src/init.c src/stream.c
           src/misc.c)

if(JPEG_FOUND)
  message(STATUS "Building libuvc with JPEG support.")
  include_directories(${JPEG_INCLUDE_DIR})
  SET(LIBUVC_HAS_JPEG TRUE)
  SET(SOURCES ${SOURCES} src/frame-mjpeg.c)
else()
  message(WARNING "JPEG not found. libuvc will not support JPEG decoding.")
endif()

if(${CMAKE_BUILD_TARGET} MATCHES "Shared")
  add_library(uvc SHARED ${SOURCES})
elseif(${CMAKE_BUILD_TARGET} MATCHES "Static")
  add_library(uvc STATIC ${SOURCES})
else()
  message( FATAL_ERROR "Invalid build type ${CMAKE_BUILD_TARGET}" )
endif()

configure_file(include/libuvc/libuvc_config.h.in
  "${CMAKE_CURRENT_BINARY_DIR}/include/libuvc/libuvc_config.h" @ONLY)

set_target_properties(uvc PROPERTIES
  PUBLIC_HEADER "include/libuvc/libuvc.h;${CMAKE_CURRENT_BINARY_DIR}/include/libuvc/libuvc_config.h")

if(JPEG_FOUND)
  target_link_libraries (uvc ${JPEG_LIBRARIES})
endif(JPEG_FOUND)

target_link_libraries(uvc ${LIBUSB_LIBRARIES})

#add_executable(test src/test.c)
#target_link_libraries(test uvc ${LIBUSB_LIBRARIES} opencv_highgui
#  opencv_core)

install(TARGETS uvc
  # IMPORTANT: Add the uvc library to the "export-set"
  EXPORT libuvcTargets
  LIBRARY DESTINATION lib COMPONENT shlib
  ARCHIVE DESTINATION lib COMPONENT lib
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/libuvc" COMPONENT dev
)

# Add all targets to the build-tree export set
export(TARGETS uvc
  FILE "${PROJECT_BINARY_DIR}/libuvcTargets.cmake")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE libuvc)

# Create the libuvcConfig.cmake and libuvcConfigVersion.cmake files
file(RELATIVE_PATH REL_INCLUDE_DIR "${ABS_INSTALL_CMAKE_DIR}"
  "${ABS_INSTALL_INCLUDE_DIR}")
# ... for the build tree
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
configure_file(libuvcConfig.cmake.in
  "${PROJECT_BINARY_DIR}/libuvcConfig.cmake" @ONLY)
# ... for the install tree
set(CONF_INCLUDE_DIRS "\${libuvc_CMAKE_DIR}/${REL_INCLUDE_DIR}")
configure_file(libuvcConfig.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libuvcConfig.cmake" @ONLY)
# ... for both
configure_file(libuvcConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/libuvcConfigVersion.cmake" @ONLY)

# Install the libuvcConfig.cmake and libuvcConfigVersion.cmake files
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libuvcConfig.cmake"
  "${PROJECT_BINARY_DIR}/libuvcConfigVersion.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)

# Install the export-set for use with the install-tree
install(EXPORT libuvcTargets
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)
