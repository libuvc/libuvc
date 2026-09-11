/** @file uvc_test_util.h
 * @brief Helpers shared by the unit tests and the fuzzers.
 *
 * libuvc's descriptor parsers work on a struct libusb_config_descriptor that
 * libusb would normally build from a real device. Nothing in libusb's public
 * API parses raw configuration bytes, so the helpers here assemble that struct
 * by hand: a config with a chosen number of interfaces, each with one
 * altsetting whose class/subclass and "extra" (class-specific descriptor)
 * bytes the caller supplies. That is all the parsers look at.
 *
 * This header is C, and is included from both the C++ unit tests and the C
 * fuzz targets.
 */
#ifndef UVC_TEST_UTIL_H
#define UVC_TEST_UTIL_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parser entry points. These are non-static in src/device.c but are not
   declared in libuvc_internal.h, so redeclare them here. */
uvc_error_t uvc_parse_vc(uvc_device_info_t *info,
                         const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vc_header(uvc_device_info_t *info,
                                const unsigned char *block, size_t block_size);
uvc_error_t uvc_scan_streaming(uvc_device_info_t *info, int interface_idx);
/* Accepts a NULL handle: it is used only for a device-specific quirk. */
uvc_error_t uvc_scan_control(uvc_device_handle_t *devh, uvc_device_info_t *info);
uvc_error_t uvc_parse_vs(uvc_device_info_t *info,
                         uvc_streaming_interface_t *stream_if,
                         const unsigned char *block, size_t block_size);
void uvc_free_device_info(uvc_device_info_t *info);

/** A synthetic config descriptor plus the allocations backing it. */
typedef struct {
  struct libusb_config_descriptor config;
  struct libusb_interface *interfaces;
  struct libusb_interface_descriptor *altsettings;
  unsigned char **extras;
  int num_interfaces;
} uvc_test_config_t;

/** Build a config descriptor with @p num_interfaces interfaces, each holding a
 * single altsetting with no endpoints and no class-specific extra bytes.
 * Use uvc_test_config_set_extra() to attach descriptor bytes to one.
 */
static inline void uvc_test_config_init(uvc_test_config_t *tc,
                                        int num_interfaces) {
  int i;

  memset(tc, 0, sizeof(*tc));
  tc->num_interfaces = num_interfaces;
  tc->interfaces = (struct libusb_interface *)
      calloc((size_t)num_interfaces, sizeof(*tc->interfaces));
  tc->altsettings = (struct libusb_interface_descriptor *)
      calloc((size_t)num_interfaces, sizeof(*tc->altsettings));
  tc->extras = (unsigned char **)
      calloc((size_t)num_interfaces, sizeof(*tc->extras));

  for (i = 0; i < num_interfaces; ++i) {
    tc->altsettings[i].bLength = LIBUSB_DT_INTERFACE_SIZE;
    tc->altsettings[i].bDescriptorType = LIBUSB_DT_INTERFACE;
    tc->altsettings[i].bInterfaceNumber = (uint8_t)i;
    tc->altsettings[i].bNumEndpoints = 0;
    tc->altsettings[i].bInterfaceClass = LIBUSB_CLASS_VIDEO;
    tc->altsettings[i].bInterfaceSubClass = UVC_SC_VIDEOSTREAMING;
    tc->interfaces[i].altsetting = &tc->altsettings[i];
    tc->interfaces[i].num_altsetting = 1;
  }

  tc->config.bLength = LIBUSB_DT_CONFIG_SIZE;
  tc->config.bDescriptorType = LIBUSB_DT_CONFIG;
  tc->config.bNumInterfaces = (uint8_t)num_interfaces;
  tc->config.interface = tc->interfaces;
}

/** Attach class-specific descriptor bytes to interface @p idx. The bytes are
 * copied, so @p data need not outlive the call. */
static inline void uvc_test_config_set_extra(uvc_test_config_t *tc, int idx,
                                             const unsigned char *data,
                                             int len) {
  free(tc->extras[idx]);
  tc->extras[idx] = NULL;

  if (len > 0) {
    /* Allocate exactly len bytes, so a sanitizer traps a read one past the
       end rather than finding slack in a rounded-up allocation. */
    tc->extras[idx] = (unsigned char *)malloc((size_t)len);
    memcpy(tc->extras[idx], data, (size_t)len);
  }

  tc->altsettings[idx].extra = tc->extras[idx];
  tc->altsettings[idx].extra_length = len;
}

static inline void uvc_test_config_free(uvc_test_config_t *tc) {
  int i;

  for (i = 0; i < tc->num_interfaces; ++i)
    free(tc->extras[i]);

  free(tc->extras);
  free(tc->altsettings);
  free(tc->interfaces);
  memset(tc, 0, sizeof(*tc));
}

/** Point a uvc_device_info_t at a synthetic config. */
static inline void uvc_test_info_init(uvc_device_info_t *info,
                                      uvc_test_config_t *tc) {
  memset(info, 0, sizeof(*info));
  info->config = &tc->config;
}

/** Release what the parsers allocated, leaving the synthetic config alone.
 *
 * uvc_free_device_info() frees info itself and calls
 * libusb_free_config_descriptor() on info->config, neither of which suits a
 * stack-allocated info pointing at a hand-built config. So hand it a heap
 * copy with the config pointer cleared; it still frees every list the parsers
 * built, which is what we want checked for leaks.
 */
static inline void uvc_test_info_free(uvc_device_info_t *info) {
  uvc_device_info_t *heap = (uvc_device_info_t *)malloc(sizeof(*heap));

  memcpy(heap, info, sizeof(*heap));
  heap->config = NULL;
  uvc_free_device_info(heap);
  memset(info, 0, sizeof(*info));
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* UVC_TEST_UTIL_H */
