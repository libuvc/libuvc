/** @file test_frame_reference.cc
 * @brief Check the pixel format converters against reference images.
 *
 * test_frame.cc checks that the converters stay inside their buffers. This
 * file checks that what they produce is right: each case converts a committed
 * source frame and compares the result byte for byte against a committed
 * expected image.
 *
 * The expected RGB comes from ImageMagick (see test/data/generate.py), so
 * these are checks against an independent implementation rather than against
 * libuvc's own past output.
 */
#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "libuvc/libuvc.h"

namespace {

#ifndef UVC_TEST_DATA_DIR
#define UVC_TEST_DATA_DIR "."
#endif

/** Absolute path to a file in test/data/. */
std::string FixturePath(const char *name) {
  return std::string(UVC_TEST_DATA_DIR "/") + name;
}

/** Read a whole fixture file; empty on failure. */
std::vector<uint8_t> ReadFixture(const std::string &path) {
  std::vector<uint8_t> data;
  std::FILE *f = std::fopen(path.c_str(), "rb");
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

/** Every converter against a committed reference image.
 *
 * The references come from ImageMagick (test/data/generate.sh), so this
 * checks the conversion is correct rather than merely unchanged. Each entry
 * names an input file, an expected-output file and a tolerance; nothing here
 * models how the converters walk their buffers, so the test cannot be wrong
 * about the code in the way a hand-derived expectation can.
 *
 * RGB and BGR allow 2 LSB: libuvc uses 2^14 fixed-point BT.601 coefficients
 * where ImageMagick works in floating point, and both round to 8 bits. The
 * luma and chroma extractions are exact -- they only demultiplex.
 */
struct ReferenceCase {
  const char *name;
  uvc_error_t (*fn)(uvc_frame_t *, uvc_frame_t *);
  uvc_frame_format in_format;
  const char *in_file;
  const char *ref_file;
  int tolerance_lsb;
};

const ReferenceCase kReferenceCases[] = {
  { "uvc_yuyv2rgb", uvc_yuyv2rgb, UVC_FRAME_FORMAT_YUYV,
    "plasma_64x32.yuyv", "plasma_64x32.rgb", 2 },
  { "uvc_yuyv2bgr", uvc_yuyv2bgr, UVC_FRAME_FORMAT_YUYV,
    "plasma_64x32.yuyv", "plasma_64x32.bgr", 2 },
  { "uvc_yuyv2y", uvc_yuyv2y, UVC_FRAME_FORMAT_YUYV,
    "plasma_64x32.yuyv", "plasma_64x32.y", 0 },
  { "uvc_yuyv2uv", uvc_yuyv2uv, UVC_FRAME_FORMAT_YUYV,
    "plasma_64x32.yuyv", "plasma_64x32.uv", 0 },
  { "uvc_uyvy2rgb", uvc_uyvy2rgb, UVC_FRAME_FORMAT_UYVY,
    "plasma_64x32.uyvy", "plasma_64x32.rgb", 2 },
  { "uvc_uyvy2bgr", uvc_uyvy2bgr, UVC_FRAME_FORMAT_UYVY,
    "plasma_64x32.uyvy", "plasma_64x32.bgr", 2 },
};

class ReferenceImageTest : public ::testing::TestWithParam<ReferenceCase> {};

std::string ReferenceName(
    const ::testing::TestParamInfo<ReferenceCase> &info) {
  return info.param.name;
}

TEST_P(ReferenceImageTest, MatchesReferenceImage) {
  constexpr uint32_t kW = 64, kH = 32;
  const ReferenceCase &c = GetParam();

  std::vector<uint8_t> src = ReadFixture(FixturePath(c.in_file));
  ASSERT_EQ(src.size(), size_t(kW) * kH * 2)
      << "could not read " << c.in_file
      << " -- regenerate with test/data/generate.sh";

  std::vector<uint8_t> ref = ReadFixture(FixturePath(c.ref_file));
  ASSERT_FALSE(ref.empty())
      << "could not read " << c.ref_file
      << " -- regenerate with test/data/generate.sh";

  FramePtr in(uvc_allocate_frame(src.size()));
  ASSERT_NE(in, nullptr);
  std::memcpy(in->data, src.data(), src.size());
  in->width = kW;
  in->height = kH;
  in->frame_format = c.in_format;
  in->step = 0;

  FramePtr out(uvc_allocate_frame(0));
  ASSERT_NE(out, nullptr);
  ASSERT_EQ(c.fn(in.get(), out.get()), UVC_SUCCESS);
  ASSERT_GE(out->data_bytes, ref.size());

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

  EXPECT_LE(worst, c.tolerance_lsb)
      << c.name << " differs from " << c.ref_file << " by " << worst
      << " at byte " << worst_at << ": got " << int(got[worst_at])
      << ", expected " << int(ref[worst_at]);
}

INSTANTIATE_TEST_SUITE_P(Converters, ReferenceImageTest,
                         ::testing::ValuesIn(kReferenceCases), ReferenceName);

}  // namespace
