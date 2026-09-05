/** @file test_frame_mjpeg.cc
 * @brief Tests for the MJPEG decoder in src/frame-mjpeg.c.
 *
 * Two things are checked: that a well-formed frame decodes to the right
 * pixels, and that a frame whose JPEG is larger than its declared geometry
 * does not make libjpeg write past the output buffer.
 *
 * The bounds tests here do not rely on a sanitizer. The offending write
 * happens inside libjpeg, which is not built with one, so AddressSanitizer
 * never sees it -- these use a canary region after the frame instead.
 */
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_config.h"

#ifdef LIBUVC_HAS_JPEG

namespace {

#ifndef UVC_TEST_DATA_DIR
#define UVC_TEST_DATA_DIR "."
#endif

std::string FixturePath(const char *name) {
  return std::string(UVC_TEST_DATA_DIR "/") + name;
}

std::vector<uint8_t> ReadFixture(const char *name) {
  std::vector<uint8_t> data;
  std::FILE *f = std::fopen(FixturePath(name).c_str(), "rb");
  if (!f) return data;
  uint8_t buf[4096];
  size_t n;
  while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
    data.insert(data.end(), buf, buf + n);
  std::fclose(f);
  return data;
}

struct FrameDeleter {
  void operator()(uvc_frame_t *f) const { if (f) uvc_free_frame(f); }
};
using FramePtr = std::unique_ptr<uvc_frame_t, FrameDeleter>;

/** An MJPEG input frame holding @p jpeg, declaring @p width x @p height. */
FramePtr MakeMjpegFrame(const std::vector<uint8_t> &jpeg, uint32_t width,
                        uint32_t height) {
  FramePtr f(uvc_allocate_frame(jpeg.size()));
  if (!f) return f;
  std::memcpy(f->data, jpeg.data(), jpeg.size());
  f->width = width;
  f->height = height;
  f->frame_format = UVC_FRAME_FORMAT_MJPEG;
  f->step = 0;
  return f;
}

/* ------------------------------------------------------------ correctness */

/** A frame whose JPEG matches its declared size decodes to the reference.
 *
 * The tolerance is 6 levels: libuvc asks libjpeg for JDCT_IFAST, a
 * deliberately approximate IDCT, so it does not reproduce a reference
 * decode exactly. The observed maximum is 4 for RGB and 5 for grayscale.
 */
TEST(MjpegDecode, MatchesReferenceImage) {
  constexpr uint32_t kW = 64, kH = 32;
  constexpr int kTolerance = 6;

  const struct {
    const char *name;
    uvc_error_t (*fn)(uvc_frame_t *, uvc_frame_t *);
    const char *ref_file;
    int bpp;
  } cases[] = {
    { "uvc_mjpeg2rgb", uvc_mjpeg2rgb, "plasma_64x32.jpg.rgb", 3 },
    { "uvc_mjpeg2gray", uvc_mjpeg2gray, "plasma_64x32.jpg.gray", 1 },
  };

  std::vector<uint8_t> jpeg = ReadFixture("plasma_64x32.jpg");
  ASSERT_FALSE(jpeg.empty()) << "regenerate with test/data/generate.py";

  for (const auto &c : cases) {
    std::vector<uint8_t> ref = ReadFixture(c.ref_file);
    ASSERT_EQ(ref.size(), size_t(kW) * kH * c.bpp) << c.ref_file;

    auto in = MakeMjpegFrame(jpeg, kW, kH);
    ASSERT_NE(in, nullptr);
    FramePtr out(uvc_allocate_frame(0));
    ASSERT_NE(out, nullptr);

    ASSERT_EQ(c.fn(in.get(), out.get()), UVC_SUCCESS) << c.name;
    ASSERT_EQ(out->width, kW);
    ASSERT_EQ(out->height, kH);

    const auto *got = static_cast<const uint8_t *>(out->data);
    int worst = 0;
    size_t worst_at = 0;
    for (size_t i = 0; i < ref.size(); ++i) {
      int err = std::abs(int(got[i]) - int(ref[i]));
      if (err > worst) {
        worst = err;
        worst_at = i;
      }
    }
    EXPECT_LE(worst, kTolerance)
        << c.name << " differs from " << c.ref_file << " by " << worst
        << " at byte " << worst_at << ": got " << int(got[worst_at])
        << ", expected " << int(ref[worst_at]);
  }
}

/** A frame that is not MJPEG must be refused. */
TEST(MjpegDecode, WrongInputFormatRejected) {
  std::vector<uint8_t> jpeg = ReadFixture("plasma_64x32.jpg");
  ASSERT_FALSE(jpeg.empty());

  auto in = MakeMjpegFrame(jpeg, 64, 32);
  ASSERT_NE(in, nullptr);
  in->frame_format = UVC_FRAME_FORMAT_YUYV;

  FramePtr out(uvc_allocate_frame(0));
  ASSERT_NE(out, nullptr);

  EXPECT_EQ(uvc_mjpeg2rgb(in.get(), out.get()), UVC_ERROR_INVALID_PARAM);
  EXPECT_EQ(uvc_mjpeg2gray(in.get(), out.get()), UVC_ERROR_INVALID_PARAM);
}

/** Garbage that is not a JPEG at all must fail rather than crash. */
TEST(MjpegDecode, GarbageInputRejected) {
  std::vector<uint8_t> garbage(4096);
  for (size_t i = 0; i < garbage.size(); ++i)
    garbage[i] = static_cast<uint8_t>(i * 7 + 13);

  auto in = MakeMjpegFrame(garbage, 64, 32);
  ASSERT_NE(in, nullptr);
  FramePtr out(uvc_allocate_frame(0));
  ASSERT_NE(out, nullptr);

  EXPECT_NE(uvc_mjpeg2rgb(in.get(), out.get()), UVC_SUCCESS);
}

/* ----------------------------------------------------------- output bounds */

/** How many bytes a converter writes past the end of the frame it was given.
 *
 * uvc_mjpeg_convert() sizes nothing itself: it hands libjpeg a row pointer
 * computed as out->data + line * out->step, with step derived from the
 * frame's *declared* width, and loops until libjpeg's own output_height. A
 * JPEG bigger than the declared geometry therefore runs off the end.
 *
 * A caller-owned buffer with a poisoned tail catches that; a sanitizer does
 * not, because the write happens inside libjpeg.
 */
size_t BytesWrittenPastFrame(uvc_error_t (*fn)(uvc_frame_t *, uvc_frame_t *),
                             const std::vector<uint8_t> &jpeg,
                             uint32_t declared_w, uint32_t declared_h,
                             int bpp, uvc_error_t *ret_out) {
  constexpr uint8_t kCanary = 0xA5;
  const size_t frame_bytes = size_t(declared_w) * declared_h * bpp;
  /* Generous tail: a mismatched JPEG can overrun by far more than one frame. */
  const size_t tail = 1u << 20;

  std::vector<uint8_t> mem(frame_bytes + tail, kCanary);

  auto in = MakeMjpegFrame(jpeg, declared_w, declared_h);
  if (!in) return 0;

  uvc_frame_t out;
  std::memset(&out, 0, sizeof(out));
  out.data = mem.data();
  out.data_bytes = frame_bytes;
  out.library_owns_data = 0;

  *ret_out = fn(in.get(), &out);

  size_t dirty = 0;
  for (size_t i = frame_bytes; i < mem.size(); ++i)
    if (mem[i] != kCanary) ++dirty;
  return dirty;
}

/** A truncated JPEG -- a partial USB transfer -- must stay in bounds.
 *
 * Note what this does *not* assert. libjpeg treats a premature end of file
 * as a warning rather than an error: it fills the missing scanlines with
 * grey and finishes normally, so uvc_mjpeg2rgb() reports success on a frame
 * that is half missing. Whether libuvc should surface that is a judgement
 * call about partial frames, not a memory-safety bug, so the current
 * behaviour is recorded rather than asserted against.
 */
TEST(MjpegDecode, TruncatedInputStaysInBounds) {
  std::vector<uint8_t> jpeg = ReadFixture("plasma_64x32.jpg");
  ASSERT_FALSE(jpeg.empty());
  jpeg.resize(jpeg.size() / 2);

  uvc_error_t ret = UVC_SUCCESS;
  EXPECT_EQ(BytesWrittenPastFrame(uvc_mjpeg2rgb, jpeg, 64, 32, 3, &ret), 0u);
}

/** An honest frame stays inside its buffer. */
TEST(MjpegBounds, MatchingGeometryStaysInBounds) {
  std::vector<uint8_t> jpeg = ReadFixture("plasma_64x32.jpg");
  ASSERT_FALSE(jpeg.empty());

  uvc_error_t ret = UVC_SUCCESS;
  EXPECT_EQ(BytesWrittenPastFrame(uvc_mjpeg2rgb, jpeg, 64, 32, 3, &ret), 0u);
  EXPECT_EQ(ret, UVC_SUCCESS);
}

/** A JPEG larger than the declared geometry must not be decoded into a
 * buffer sized for that geometry.
 *
 * The frame's width and height come from the format negotiated with the
 * camera; the JPEG's dimensions come from the frame data itself, which a
 * malfunctioning or hostile device controls independently. libuvc currently
 * trusts the latter to size its writes while sizing the buffer from the
 * former, so the decode writes past the end and still reports success.
 */
TEST(MjpegBounds, OversizedJpegDoesNotOverflowOutput) {
  const struct {
    const char *file;
    uint32_t real_w, real_h;
  } cases[] = {
    { "oversize_128x64.jpg", 128, 64 },
    { "oversize_64x256.jpg", 64, 256 },
  };

  for (const auto &c : cases) {
    std::vector<uint8_t> jpeg = ReadFixture(c.file);
    ASSERT_FALSE(jpeg.empty()) << c.file;

    uvc_error_t ret = UVC_SUCCESS;
    size_t past = BytesWrittenPastFrame(uvc_mjpeg2rgb, jpeg, 64, 32, 3, &ret);

    EXPECT_EQ(past, 0u)
        << c.file << " (really " << c.real_w << "x" << c.real_h
        << ") declared as 64x32 wrote " << past
        << " bytes past the end of the output frame";

    /* Whatever it does about the pixels, it must not claim success. */
    EXPECT_NE(ret, UVC_SUCCESS)
        << c.file << " declared as 64x32 returned success";
  }
}

/** The same for the grayscale path, which sizes its buffer differently. */
TEST(MjpegBounds, OversizedJpegDoesNotOverflowGrayOutput) {
  std::vector<uint8_t> jpeg = ReadFixture("oversize_64x256.jpg");
  ASSERT_FALSE(jpeg.empty());

  uvc_error_t ret = UVC_SUCCESS;
  size_t past = BytesWrittenPastFrame(uvc_mjpeg2gray, jpeg, 64, 32, 1, &ret);

  EXPECT_EQ(past, 0u)
      << "a 64x256 JPEG declared as 64x32 wrote " << past
      << " bytes past the end of the GRAY8 output frame";
  EXPECT_NE(ret, UVC_SUCCESS);
}

}  // namespace

#endif /* LIBUVC_HAS_JPEG */
