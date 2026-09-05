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

gtest_discover_tests(test_frame
  DISCOVERY_TIMEOUT 30
  PROPERTIES TIMEOUT 60
)
