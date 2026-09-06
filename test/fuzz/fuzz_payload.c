/** @file fuzz_payload.c
 * @brief Fuzz the isochronous/bulk payload header parser in src/stream.c.
 *
 * _uvc_process_payload() runs on every USB packet of every frame, so unlike
 * the descriptor parsers it is on the hot path rather than the setup path.
 * The payload header is device-supplied: a length byte, a flags byte, and
 * then optional PTS and SCR fields whose presence the flags announce.
 *
 * Nothing here needs USB. The function reads a handful of fields from the
 * stream handle -- the frame size it negotiated, the two output buffers and
 * the metadata buffer -- and calls _uvc_swap_buffers() on end-of-frame, which
 * only swaps pointers under a mutex. So a hand-built uvc_stream_handle_t is
 * enough, and the fuzzer drives the real function.
 *
 * Build: see test/fuzz/README.md, or -DBUILD_FUZZERS=ON with clang.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

void _uvc_process_payload(uvc_stream_handle_t *strmh, uint8_t *payload,
                          size_t payload_len);

/* A frame size small enough that a run stays cheap, large enough that a
   payload can fill one and trigger the end-of-frame path. */
#define FUZZ_FRAME_BYTES 4096
#define FUZZ_MAX_PAYLOAD 8192

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  uvc_stream_handle_t strmh;
  uvc_device_handle_t devh;
  uint8_t *payload;
  int is_isight;

  if (size < 2 || size > FUZZ_MAX_PAYLOAD)
    return 0;

  /* Byte 0 selects the iSight quirk, which takes a different path through
     the header logic and is otherwise hard to reach. */
  is_isight = data[0] & 1;
  ++data;
  --size;

  memset(&devh, 0, sizeof(devh));
  devh.is_isight = is_isight;

  memset(&strmh, 0, sizeof(strmh));
  strmh.devh = &devh;
  strmh.cur_ctrl.dwMaxVideoFrameSize = FUZZ_FRAME_BYTES;
  strmh.outbuf = malloc(FUZZ_FRAME_BYTES);
  strmh.holdbuf = malloc(FUZZ_FRAME_BYTES);
  strmh.meta_outbuf = malloc(LIBUVC_XFER_META_BUF_SIZE);
  strmh.meta_holdbuf = malloc(LIBUVC_XFER_META_BUF_SIZE);

  if (!strmh.outbuf || !strmh.holdbuf ||
      !strmh.meta_outbuf || !strmh.meta_holdbuf)
    goto out;

  pthread_mutex_init(&strmh.cb_mutex, NULL);
  pthread_cond_init(&strmh.cb_cond, NULL);

  /* Copy the payload to its own exact-sized allocation, so a read past the
     end lands in a redzone rather than in the rest of the fuzzer's input. */
  payload = malloc(size);
  if (!payload) {
    pthread_cond_destroy(&strmh.cb_cond);
    pthread_mutex_destroy(&strmh.cb_mutex);
    goto out;
  }
  memcpy(payload, data, size);

  /* Twice: the parser carries state between packets -- got_bytes, the frame
     ID used to detect a missing end-of-frame, the metadata offset -- and a
     single call would never exercise it. */
  _uvc_process_payload(&strmh, payload, size);
  _uvc_process_payload(&strmh, payload, size);

  free(payload);
  pthread_cond_destroy(&strmh.cb_cond);
  pthread_mutex_destroy(&strmh.cb_mutex);

out:
  free(strmh.outbuf);
  free(strmh.holdbuf);
  free(strmh.meta_outbuf);
  free(strmh.meta_holdbuf);
  return 0;
}
