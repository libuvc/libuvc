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
 * Two entry points are driven, both real library functions -- nothing here
 * re-implements libuvc's own descriptor walk, so the fuzzer cannot drift
 * from the code it is testing:
 *
 *   uvc_scan_streaming()  walks the block list itself, and is the function
 *                         in the #300 stack trace.
 *   uvc_parse_vc()        one VideoControl block, reached directly.
 *
 * uvc_scan_control() would be the third, but it needs a uvc_device_handle_t
 * for the TIS-camera quirk, which means a real USB device.
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
     which one to scan. Varying the two independently is what reaches #300:
     that bug needs baInterfaceNr[] in a VC header to name an interface the
     configuration does not have. */
  num_interfaces = (data[0] % UVC_FUZZ_MAX_INTERFACES) + 1;
  target = data[1] % (UVC_FUZZ_MAX_INTERFACES + 1);
  data += 2;
  size -= 2;

  uvc_test_config_init(&tc, num_interfaces);
  uvc_test_config_set_extra(&tc, 0, data, (int)size);
  uvc_test_info_init(&info, &tc);

  /* Walks the block list itself, including the bLength handling that decides
     how far it advances. */
  uvc_scan_streaming(NULL, &info, target);

  /* And the VideoControl side, one block, no walking. Every caller checks
     for at least three bytes first ("parseX needs to see buf[0,2]"), so
     honour that contract rather than reporting a short read the library
     cannot actually receive. */
  if (size >= 3)
    uvc_parse_vc(NULL, &info, data, size);

  uvc_test_info_free(&info);
  uvc_test_config_free(&tc);

  return 0;
}
