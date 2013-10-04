cmake_minimum_required(VERSION 2.6)
project(libuvc)

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

target_link_libraries(uvc ${LIBUSB_LIBRARY_NAMES})

#add_executable(test src/test.c)
#target_link_libraries(test uvc ${LIBUSB_LIBRARY_NAMES} opencv_highgui
#  opencv_core)

set(CONF_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
set(CONF_LIBRARY "${CMAKE_INSTALL_PREFIX}/lib/libuvc.so")

configure_file(libuvcConfig.cmake.in ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libuvcConfig.cmake)

install(TARGETS uvc
  LIBRARY DESTINATION lib
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/libuvc/libuvc.h"
  DESTINATION include/libuvc)

install(FILES "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libuvcConfig.cmake" DESTINATION "${INSTALL_CMAKE_DIR}")
