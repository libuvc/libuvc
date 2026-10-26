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
 * interface, walks its class-specific blocks, dispatches each to the parsers,
 * and follows a VideoControl header into uvc_scan_streaming() for every
 * interface it names -- so one call covers the whole path in the #300 report
 * without the fuzzer re-implementing any of libuvc's descriptor walking.
 *
 * It takes a uvc_device_handle_t only for a device-specific quirk lookup and
 * tolerates NULL, which is what makes it reachable without a USB device.
 *
 * The same fuzzed block is installed on two interfaces: interface 0, the
 * VideoControl one that uvc_scan_control() selects, and one other, so that
 * the VideoStreaming parsers see fuzzed bytes too rather than the empty
 * `extra` they would get otherwise. Reaching them still depends on the block
 * happening to form a VideoControl header naming that interface, so
 * uvc_scan_streaming() is also called directly afterwards -- the same
 * parsers, minus the many mutations it takes to stumble on a valid header.
 *
 * Build: see test/fuzz/README.md, or -DBUILD_FUZZERS=ON with clang.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "uvc_test_util.h"

/* A single block is bounded by bLength being one byte, but `extra` is a list
 * of them and has no such limit: the Logitech HD Pro 920 in cameras/ carries
 * ~153 blocks totalling ~3.5 kB, and the QuickCam Pro 9000 ~2.6 kB. The cap
 * here is well past both. Note libFuzzer stops at 4096 bytes of its own
 * accord unless -max_len says otherwise, so reaching this bound takes
 * -max_len=16384 on the command line; see README.md. */
#define UVC_FUZZ_MAX_INPUT 16384
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

  /* One configuration for both calls. Interface 0 is VideoControl, so
     uvc_scan_control() selects it; `target` also carries the block, so a
     VideoControl header naming it drives the VideoStreaming parsers over
     fuzzed bytes rather than an empty descriptor. */
  uvc_test_config_init(&tc, num_interfaces);
  tc.altsettings[0].bInterfaceSubClass = UVC_SC_VIDEOCONTROL;
  uvc_test_config_set_extra(&tc, 0, data, (int)size);
  if (target < num_interfaces && target != 0)
    uvc_test_config_set_extra(&tc, target, data, (int)size);
  uvc_test_info_init(&info, &tc);

  uvc_scan_control(NULL, &info);

  /* And the VideoStreaming side directly. uvc_scan_control() gets here only
     via a header naming a valid interface, which costs the fuzzer a great
     many mutations to produce; `target` is deliberately allowed to exceed
     num_interfaces, which is what #300 turns on. Reuses the parsed info
     rather than rebuilding it -- uvc_scan_control() appends to the same
     lists, so this exercises the two running against shared state, as they
     do inside uvc_get_device_info(). */
  uvc_scan_streaming(&info, target);

  uvc_test_info_free(&info);
  uvc_test_config_free(&tc);

  return 0;
}
