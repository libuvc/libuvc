/** @file fuzz_descriptor.c
 * @brief Fuzz the USB descriptor parsers in src/device.c.
 *
 * libuvc parses descriptors supplied by the device before validating any of
 * them, so a malicious or merely broken camera controls this input entirely.
 * That makes these parsers the library's main attack surface, and the shape
 * of the reproducer in libuvc/libuvc#300.
 *
 * The fuzzer's bytes become the class-specific ("extra") descriptor block of
 * an interface in a synthetic USB configuration, which is exactly where they
 * would come from on a real device.
 *
 * The entry point is uvc_scan_control(), the same function uvc_open() calls
 * once it has a configuration descriptor. It picks the VideoControl
 * interface, walks its class-specific blocks and dispatches each to the
 * parsers, so one call covers the whole path in the #300 report without the
 * fuzzer re-implementing any of libuvc's own descriptor walking.
 *
 * It takes a uvc_device_handle_t only for a device-specific quirk lookup and
 * tolerates NULL, which is what makes it reachable without a USB device.
 *
 * uvc_scan_streaming() is driven separately: uvc_scan_control() only reaches
 * it through a VideoControl header naming an interface, so calling it
 * directly gets there in far fewer mutations.
 *
 * Build: see test/fuzz/README.md, or -DBUILD_FUZZERS=ON with clang.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "uvc_test_util.h"

/* Descriptor blocks are bounded by bLength being a single byte, so nothing
 * beyond a few hundred bytes explores new parser states. */
#define UVC_FUZZ_MAX_INPUT 4096
#define UVC_FUZZ_MAX_INTERFACES 8

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  uvc_test_config_t tc;
  uvc_device_info_t info;
  int num_interfaces, target;

  if (size < 3 || size > UVC_FUZZ_MAX_INPUT)
    return 0;

  /* Byte 0 picks how many interfaces the configuration declares and byte 1
     which one uvc_scan_streaming() is pointed at. Varying the two
     independently is what reaches #300: that bug needs an interface index
     the configuration does not actually have. */
  num_interfaces = (data[0] % UVC_FUZZ_MAX_INTERFACES) + 1;
  target = data[1] % (UVC_FUZZ_MAX_INTERFACES + 1);
  data += 2;
  size -= 2;

  /* Interface 0 is the VideoControl interface, so uvc_scan_control() selects
     it and parses the fuzzed block. */
  uvc_test_config_init(&tc, num_interfaces);
  tc.altsettings[0].bInterfaceSubClass = UVC_SC_VIDEOCONTROL;
  uvc_test_config_set_extra(&tc, 0, data, (int)size);
  uvc_test_info_init(&info, &tc);

  uvc_scan_control(NULL, &info);

  uvc_test_info_free(&info);
  uvc_test_config_free(&tc);

  /* The VideoStreaming side, reached directly rather than through a
     VideoControl header that has to name a valid interface first. */
  uvc_test_config_init(&tc, num_interfaces);
  uvc_test_config_set_extra(&tc, 0, data, (int)size);
  uvc_test_info_init(&info, &tc);

  uvc_scan_streaming(&info, target);

  uvc_test_info_free(&info);
  uvc_test_config_free(&tc);

  return 0;
}
