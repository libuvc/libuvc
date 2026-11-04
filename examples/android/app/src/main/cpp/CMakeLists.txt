cmake_minimum_required(VERSION 3.22.1...3.31)
project(uvcexample C)

include(FetchContent)

# libusb ----------------------------------------------------------------------
#
# libusb does not build with CMake, so compile its Linux backend directly, as
# its own android/jni/libusb.mk does. Keep it a shared library: libusb is
# LGPL-2.1, and shipping it as its own .so is the simple way to let users
# replace it.
FetchContent_Declare(libusb
  URL https://github.com/libusb/libusb/releases/download/v1.0.30/libusb-1.0.30.tar.bz2
  URL_HASH SHA256=fea36f34f9156400209595e300840767ab1a385ede1dc7ee893015aea9c6dbaf
)
FetchContent_MakeAvailable(libusb)

add_library(usb-1.0 SHARED
  ${libusb_SOURCE_DIR}/libusb/core.c
  ${libusb_SOURCE_DIR}/libusb/descriptor.c
  ${libusb_SOURCE_DIR}/libusb/hotplug.c
  ${libusb_SOURCE_DIR}/libusb/io.c
  ${libusb_SOURCE_DIR}/libusb/strerror.c
  ${libusb_SOURCE_DIR}/libusb/sync.c
  ${libusb_SOURCE_DIR}/libusb/os/events_posix.c
  ${libusb_SOURCE_DIR}/libusb/os/linux_netlink.c
  ${libusb_SOURCE_DIR}/libusb/os/linux_usbfs.c
  ${libusb_SOURCE_DIR}/libusb/os/threads_posix.c
)
target_include_directories(usb-1.0
  PUBLIC ${libusb_SOURCE_DIR}/libusb
  # android/config.h stands in for the config.h that ./configure writes.
  PRIVATE ${libusb_SOURCE_DIR}/android ${libusb_SOURCE_DIR}/libusb/os
)
target_compile_options(usb-1.0 PRIVATE -fvisibility=hidden)
target_link_libraries(usb-1.0 PRIVATE log)

# libuvc only looks for libusb with pkg-config when there is no LibUSB::LibUSB
# target yet. Provide one, as an IMPORTED target: libuvc's install(EXPORT)
# rejects a dependency on a plain target that is not exported with it.
add_library(LibUSB::LibUSB INTERFACE IMPORTED)
target_link_libraries(LibUSB::LibUSB INTERFACE usb-1.0)

# libuvc ----------------------------------------------------------------------
#
# This example builds the libuvc tree it lives in. In an app of your own, point
# LIBUVC_SOURCE_DIR at a submodule, or fetch libuvc with FetchContent too.
set(LIBUVC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..
  CACHE PATH "libuvc source tree")

set(CMAKE_BUILD_TARGET Static)
set(BUILD_EXAMPLE OFF)
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
