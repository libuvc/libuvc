/** @file test_stream.cc
 * @brief Tests for the payload header parser in src/stream.c.
 *
 * _uvc_process_payload() runs on every USB packet of every frame, and the
 * header it reads is entirely device-supplied: a length, a flags byte, and
 * optional PTS and SCR fields whose presence the flags announce.
 *
 * Nothing here needs USB. The function reads a few fields from the stream
 * handle and calls _uvc_swap_buffers() on end-of-frame, which only swaps
 * pointers under a mutex, so a hand-built handle is enough.
 */
#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <vector>

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

extern "C" void _uvc_process_payload(uvc_stream_handle_t *strmh,
                                     uint8_t *payload, size_t payload_len);

namespace {

constexpr size_t kFrameBytes = 4096;

/** A stream handle with just enough set up to run the payload parser. */
class StreamFixture {
 public:
  StreamFixture(int is_isight = 0) {
    std::memset(&devh_, 0, sizeof(devh_));
    devh_.is_isight = is_isight;

    std::memset(&strmh_, 0, sizeof(strmh_));
    strmh_.devh = &devh_;
    strmh_.cur_ctrl.dwMaxVideoFrameSize = kFrameBytes;
    outbuf_.resize(kFrameBytes);
    holdbuf_.resize(kFrameBytes);
    meta_outbuf_.resize(LIBUVC_XFER_META_BUF_SIZE);
    meta_holdbuf_.resize(LIBUVC_XFER_META_BUF_SIZE);
    strmh_.outbuf = outbuf_.data();
    strmh_.holdbuf = holdbuf_.data();
    strmh_.meta_outbuf = meta_outbuf_.data();
    strmh_.meta_holdbuf = meta_holdbuf_.data();

    pthread_mutex_init(&strmh_.cb_mutex, nullptr);
    pthread_cond_init(&strmh_.cb_cond, nullptr);
  }

  ~StreamFixture() {
    pthread_cond_destroy(&strmh_.cb_cond);
    pthread_mutex_destroy(&strmh_.cb_mutex);
  }

  /** Feed a payload, copied to an exact-sized buffer so an overread lands
   * in a redzone rather than in adjacent test data. */
  void Process(const std::vector<uint8_t> &payload) {
    std::vector<uint8_t> exact(payload);
    _uvc_process_payload(&strmh_, exact.data(), exact.size());
  }

  uvc_stream_handle_t *handle() { return &strmh_; }

 private:
  uvc_device_handle_t devh_;
  uvc_stream_handle_t strmh_;
  std::vector<uint8_t> outbuf_, holdbuf_, meta_outbuf_, meta_holdbuf_;
};

/* The PTS and SCR flags announce fields that follow the two-byte header, but
 * setting a flag does not make the bytes exist: bLength has to cover them.
 * The parser checked header_len against payload_len and then read the fields
 * regardless, so a two-byte packet with either flag set read past the end.
 * Found by fuzz_payload within seconds. */
TEST(PayloadHeader, PtsFlagOnShortHeaderIsRejected) {
  StreamFixture f;
  /* bLength=2, bmHeaderInfo=PTS. No room for the 4-byte PTS. */
  f.Process({0x02, UVC_STREAM_PTS});
  EXPECT_EQ(f.handle()->pts, 0u);
}

TEST(PayloadHeader, ScrFlagOnShortHeaderIsRejected) {
  StreamFixture f;
  /* bLength=2, bmHeaderInfo=SCR. No room for the 6-byte SCR. */
  f.Process({0x02, UVC_STREAM_SCR});
  EXPECT_EQ(f.handle()->last_scr, 0u);
}

TEST(PayloadHeader, PtsAndScrTogetherOnShortHeaderAreRejected) {
  StreamFixture f;
  f.Process({0x02, uint8_t(UVC_STREAM_PTS | UVC_STREAM_SCR)});
  EXPECT_EQ(f.handle()->pts, 0u);
  EXPECT_EQ(f.handle()->last_scr, 0u);
}

/** A header that does declare room for PTS is parsed. */
TEST(PayloadHeader, WellFormedPtsIsAccepted) {
  StreamFixture f;
  /* bLength=6: two header bytes plus a 4-byte PTS of 0x04030201. */
  f.Process({0x06, UVC_STREAM_PTS, 0x01, 0x02, 0x03, 0x04});
  EXPECT_EQ(f.handle()->pts, 0x04030201u);
}

/** And one declaring both fields. */
TEST(PayloadHeader, WellFormedPtsAndScrAreAccepted) {
  StreamFixture f;
  /* bLength=12: 2 + 4 (PTS) + 6 (SCR). */
  f.Process({0x0c, uint8_t(UVC_STREAM_PTS | UVC_STREAM_SCR),
             0x01, 0x02, 0x03, 0x04,
             0x05, 0x06, 0x07, 0x08, 0x09, 0x0a});
  EXPECT_EQ(f.handle()->pts, 0x04030201u);
  EXPECT_EQ(f.handle()->last_scr, 0x08070605u);
}

/** A header longer than the packet is refused outright. */
TEST(PayloadHeader, HeaderLongerThanPayloadIsRejected) {
  StreamFixture f;
  f.Process({0xff, 0x00, 0x00, 0x00});
  EXPECT_EQ(f.handle()->got_bytes, 0u);
}

/** The error bit means the packet is to be dropped, not parsed. */
TEST(PayloadHeader, ErrorBitDropsThePacket) {
  StreamFixture f;
  f.Process({0x02, UVC_STREAM_ERR, 0xaa, 0xbb, 0xcc});
  EXPECT_EQ(f.handle()->got_bytes, 0u);
}

/** An empty transfer is ignored rather than dereferenced. */
TEST(PayloadHeader, EmptyPayloadIsIgnored) {
  StreamFixture f;
  f.Process({});
  EXPECT_EQ(f.handle()->got_bytes, 0u);
}

/** Image data is copied out and counted. */
TEST(PayloadHeader, ImageDataIsAccumulated) {
  StreamFixture f;
  f.Process({0x02, 0x00, 0x11, 0x22, 0x33, 0x44});
  EXPECT_EQ(f.handle()->got_bytes, 4u);
}

/* got_bytes is clamped against dwMaxVideoFrameSize, but the clamp subtracted
 * the other way round, which underflows once the frame is exactly full --
 * both operands are size_t. Feed a full frame, then more. */
TEST(PayloadHeader, DataBeyondFrameSizeDoesNotUnderflowTheClamp) {
  StreamFixture f;

  std::vector<uint8_t> full{0x02, 0x00};
  full.resize(2 + kFrameBytes, 0x5a);
  f.Process(full);

  /* The frame is now full or has been published; either way another packet
     must not compute a negative amount of space left. */
  f.Process({0x02, 0x00, 0x11, 0x22, 0x33, 0x44});
  EXPECT_LE(f.handle()->got_bytes, kFrameBytes);
}

/* Metadata is clamped the same way and had the same underflow. */
TEST(PayloadHeader, MetadataBeyondBufferDoesNotUnderflowTheClamp) {
  StreamFixture f;

  /* bLength claims a long header, all of it metadata past the 2-byte
     prefix. Repeat until the metadata buffer is well past full. */
  std::vector<uint8_t> packet{0xff, 0x00};
  packet.resize(0xff, 0x5a);

  for (int i = 0; i < (LIBUVC_XFER_META_BUF_SIZE / 0xff) + 4; ++i)
    f.Process(packet);

  EXPECT_LE(f.handle()->meta_got_bytes, size_t(LIBUVC_XFER_META_BUF_SIZE));
}

/** The iSight quirk takes a different path through the same header logic. */
TEST(PayloadHeader, IsightShortPayloadIsSafe) {
  StreamFixture f(/*is_isight=*/1);
  f.Process({0x02, UVC_STREAM_SCR});
  f.Process({0x02, uint8_t(UVC_STREAM_PTS | UVC_STREAM_SCR)});
  EXPECT_LE(f.handle()->got_bytes, kFrameBytes);
}

}  // namespace
