cmake_minimum_required(VERSION 2.6)
project(libuvc)

find_library(LIBUSB_LIBRARY_NAMES usb-1.0
	PATHS /opt/local/lib)

find_path(LIBUSB_INCLUDE_DIR libusb-1.0/libusb.h
	PATHS /opt/local/include)

SET(CMAKE_C_FLAGS_DEBUG "-g -DUVC_DEBUGGING")

include_directories(${libuvc_SOURCE_DIR}/include ${LIBUSB_INCLUDE_DIR})
link_directories(${LIBUSB_LIBRARY_NAMES})

add_library(uvc SHARED src/ctrl.c src/device.c src/diag.c
                       src/frame.c src/init.c src/stream.c
                       src/misc.c)

target_link_libraries(uvc ${LIBUSB_LIBRARY_NAMES})

#add_executable(test src/test.c)
#target_link_libraries(test uvc ${LIBUSB_LIBRARY_NAMES} opencv_highgui
#  opencv_core)

install(TARGETS uvc
  LIBRARY DESTINATION lib
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/utlist.h"
  DESTINATION include)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/libuvc/libuvc.h"
  DESTINATION include/libuvc)

