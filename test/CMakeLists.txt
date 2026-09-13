# Unit tests. Opt-in via BUILD_TESTING, so a plain `cmake ..` still needs
# nothing but a C compiler and libusb.

enable_language(CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED)
include(GoogleTest)

if(NOT TARGET uvc_static)
  message(FATAL_ERROR
    "Tests need the static library. Configure with "
    "-DCMAKE_BUILD_TARGET=Both (the default) or Static.")
endif()

add_executable(test_frame test_frame.cc)
target_link_libraries(test_frame PRIVATE uvc_static GTest::gtest_main)

# Reference images are read at run time, so the test needs to know where
# test/data/ is regardless of the working directory it is invoked from.
add_executable(test_frame_reference test_frame_reference.cc)
target_link_libraries(test_frame_reference PRIVATE uvc_static GTest::gtest_main)
target_compile_definitions(test_frame_reference
  PRIVATE UVC_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

# Compiles to an empty translation unit when libuvc was built without JPEG
# support, so it needs no guard here.
add_executable(test_frame_mjpeg test_frame_mjpeg.cc)
target_link_libraries(test_frame_mjpeg PRIVATE uvc_static GTest::gtest_main)
target_compile_definitions(test_frame_mjpeg
  PRIVATE UVC_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

foreach(test_name IN ITEMS test_frame test_frame_reference test_frame_mjpeg)
  gtest_discover_tests(${test_name}
    DISCOVERY_TIMEOUT 30
    PROPERTIES TIMEOUT 60
  )
endforeach()
