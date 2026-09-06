# Unit tests and fuzz targets. Opt-in via BUILD_TESTING, so a plain
# `cmake ..` still needs nothing but a C compiler and libusb.

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

# The descriptor parsers are internal symbols that the shared library does not
# export, so this links the static library and includes libuvc_internal.h.
add_executable(test_descriptor test_descriptor.cc)
target_link_libraries(test_descriptor
  # LibUSB explicitly: libuvc_internal.h includes <libusb.h>, and uvc_static
  # links LibUSB privately so its include path does not propagate.
  PRIVATE uvc_static LibUSB::LibUSB GTest::gtest_main
)
target_include_directories(test_descriptor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Uses internal stream structures, so it needs libusb's headers too.
add_executable(test_stream test_stream.cc)
target_link_libraries(test_stream
  PRIVATE uvc_static LibUSB::LibUSB GTest::gtest_main
)
target_include_directories(test_stream PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

foreach(test_name IN ITEMS test_frame test_descriptor test_stream)
  gtest_discover_tests(${test_name}
    DISCOVERY_TIMEOUT 30
    # A zero-length descriptor makes uvc_scan_streaming() loop forever, so a
    # hang is a real outcome here, not a hypothetical.
    PROPERTIES TIMEOUT 60
  )
endforeach()

# libFuzzer targets, built only on request and only with clang. They double as
# OSS-Fuzz targets: OSS-Fuzz supplies its own $LIB_FUZZING_ENGINE, so link that
# when it is set rather than hardcoding -fsanitize=fuzzer.
if(BUILD_FUZZERS)
  if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR
      "BUILD_FUZZERS needs clang (libFuzzer); found ${CMAKE_C_COMPILER_ID}. "
      "Configure with -DCMAKE_C_COMPILER=clang.")
  endif()

  # Only the descriptor parsers are fuzzed. The pixel converters take the same
  # untrusted bytes, but they are branch-free arithmetic over the payload: the
  # only thing that varies their control flow is the geometry-vs-length
  # relationship, which test_frame.cc already enumerates exhaustively.
  foreach(fuzzer IN ITEMS fuzz_descriptor fuzz_payload)
    add_executable(${fuzzer} fuzz/${fuzzer}.c)
    target_link_libraries(${fuzzer} PRIVATE uvc_static LibUSB::LibUSB)
    target_include_directories(${fuzzer} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    if(DEFINED ENV{LIB_FUZZING_ENGINE})
      target_link_libraries(${fuzzer} PRIVATE $ENV{LIB_FUZZING_ENGINE})
    else()
      target_compile_options(${fuzzer} PRIVATE -fsanitize=fuzzer)
      target_link_options(${fuzzer} PRIVATE -fsanitize=fuzzer)
    endif()
  endforeach()

  # Replay the checked-in corpus as an ordinary test, so the regression inputs
  # are exercised on every run without anyone having to fuzz.
  foreach(fuzzer IN ITEMS fuzz_descriptor fuzz_payload)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/corpus/${fuzzer})
      add_test(NAME ${fuzzer}_corpus
        COMMAND ${fuzzer} -runs=0 ${CMAKE_CURRENT_SOURCE_DIR}/corpus/${fuzzer}
      )
    endif()
  endforeach()
endif()
