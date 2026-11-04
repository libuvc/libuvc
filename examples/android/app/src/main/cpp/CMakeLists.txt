cmake_minimum_required(VERSION 3.22.1...3.31)
project(uvcexample C)

include(FetchContent)

# libusb ----------------------------------------------------------------------
#
# The libusb project maintains a CMake build of libusb, libusb-cmake, which
# handles Android. Keep libusb a shared library: it is LGPL-2.1, and shipping
# it as its own .so is the simple way to let users replace it.
set(LIBUSB_BUILD_SHARED_LIBS ON)
set(LIBUSB_INSTALL_TARGETS OFF)
FetchContent_Declare(libusb
  URL https://github.com/libusb/libusb-cmake/archive/refs/tags/v1.0.30-0.tar.gz
  URL_HASH SHA256=c8143ceba42b7b93d484ccd09db7be261bb9b15f61dae5fdc605a01a6913bada
)
FetchContent_MakeAvailable(libusb)

# libuvc only looks for libusb with pkg-config when there is no LibUSB::LibUSB
# target yet.
add_library(LibUSB::LibUSB ALIAS usb-1.0)

# libuvc ----------------------------------------------------------------------
#
# This example builds the libuvc tree it lives in. In an app of your own, point
# LIBUVC_SOURCE_DIR at a submodule, or fetch libuvc with FetchContent too.
set(LIBUVC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..
  CACHE PATH "libuvc source tree")

set(CMAKE_BUILD_TARGET Static)
set(BUILD_EXAMPLE OFF)
# An app installs nothing, and libuvc's install rules could not refer to the
# libusb target above anyway.
set(LIBUVC_INSTALL OFF)
# MJPEG is decoded with the platform's AImageDecoder instead of libjpeg.
set(DISABLE_JPEG ON)
# Phones often fail large isochronous transfers with ENOMEM (see libuvc's
# README); smaller ones cost CPU instead. A cache variable, because libuvc
# declares it as one, which would hide a plain variable on the first
# configure: FORCE it, or a value cached by an earlier build would stick.
set(LIBUVC_PACKETS_PER_TRANSFER_MAX 8 CACHE STRING "" FORCE)
add_subdirectory(${LIBUVC_SOURCE_DIR} libuvc EXCLUDE_FROM_ALL)

# The JNI library -------------------------------------------------------------
add_library(uvcexample SHARED
  convert.c
  uvc_jni.c
)
target_compile_options(uvcexample PRIVATE
  -Wall -Wextra -Wno-unused-parameter
  # Only JNI_OnLoad needs exporting: the native methods are registered by it.
  -fvisibility=hidden
)
# Nor re-export the symbols of the static libuvc.
target_link_options(uvcexample PRIVATE -Wl,--exclude-libs,ALL)
target_link_libraries(uvcexample PRIVATE
  LibUVC::UVCStatic
  LibUSB::LibUSB
  android
  jnigraphics
  log
)
