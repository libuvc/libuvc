cmake_minimum_required(VERSION 2.6)
project(libuvc)

set(libuvc_VERSION_MAJOR 0)
set(libuvc_VERSION_MINOR 0)
set(libuvc_VERSION_PATCH 2)
set(libuvc_VERSION ${libuvc_VERSION_MAJOR}.${libuvc_VERSION_MINOR}.${libuvc_VERSION_PATCH})

find_library(LIBUSB_LIBRARY_NAMES usb-1.0
	PATHS /opt/local/lib)

find_path(LIBUSB_INCLUDE_DIR libusb-1.0/libusb.h
	PATHS /opt/local/include)

SET(CMAKE_C_FLAGS_DEBUG "-g -DUVC_DEBUGGING")

SET(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_PREFIX}/lib/cmake/libuvc" CACHE PATH
	"Installation directory for CMake files")

include_directories(${libuvc_SOURCE_DIR}/include ${LIBUSB_INCLUDE_DIR})
link_directories(${LIBUSB_LIBRARY_NAMES})

add_library(uvc SHARED src/ctrl.c src/device.c src/diag.c
                       src/frame.c src/init.c src/stream.c
                       src/misc.c)

set_target_properties(uvc PROPERTIES
  PUBLIC_HEADER "include/libuvc/libuvc.h")

target_link_libraries(uvc ${LIBUSB_LIBRARY_NAMES})

#add_executable(test src/test.c)
#target_link_libraries(test uvc ${LIBUSB_LIBRARY_NAMES} opencv_highgui
#  opencv_core)

install(TARGETS uvc
  EXPORT libuvcTargets
  LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libuvc"
)

export(TARGETS uvc
  FILE "${PROJECT_BINARY_DIR}/libuvcTargets.cmake")
export(PACKAGE libuvc)

set(CONF_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
set(CONF_LIBRARY "${CMAKE_INSTALL_PREFIX}/lib/libuvc.so")

configure_file(libuvcConfig.cmake.in ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libuvcConfig.cmake)

configure_file(libuvcConfigVersion.cmake.in ${PROJECT_BINARY_DIR}/libuvcConfigVersion.cmake @ONLY)

install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libuvcConfig.cmake"
  "${PROJECT_BINARY_DIR}/libuvcConfigVersion.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}")

install(EXPORT libuvcTargets
  DESTINATION "${INSTALL_CMAKE_DIR}")
