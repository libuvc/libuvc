/** @file test_frame.cc
 * @brief Unit tests for the pixel format converters in src/frame.c.
 *
 * A frame's width and height come from the negotiated format, but data_bytes
 * is however much the camera actually sent, so a truncated transfer leaves
 * the two inconsistent. A converter bounding its loop on the output buffer
 * rather than the input geometry then reads past the end of the input.
 *
 * Run these with -DENABLE_SANITIZERS=ON: without one, the overflow cases
 * read stale heap memory and appear to pass.
 */
#include <gtest/gtest.h>

#include <cstdio>
#include <memory>
#include <vector>

#include "libuvc/libuvc.h"

namespace {

/* ---------------------------------------------------------------- helpers */

/** Run @p body in a forked child and require a clean exit, so that one
 * sanitizer abort does not take the rest of the matrix with it. */
#define EXPECT_MEMORY_SAFE(body) \
  EXPECT_EXIT({ body; _exit(0); }, ::testing::ExitedWithCode(0), "")

/** Assert inside an EXPECT_MEMORY_SAFE body. GoogleTest's macros do not work
 * here: ASSERT_* expands to a return, which EXPECT_EXIT rejects, and EXPECT_*
 * failures die with the child. Only the exit status crosses back. */
#define REQUIRE_IN_CHILD(cond) \
  do { \
    if (!(cond)) { \
      std::fprintf(stderr, "REQUIRE_IN_CHILD failed: %s at %s:%d\n", \
                   #cond, __FILE__, __LINE__); \
      _exit(1); \
    } \
  } while (0)


struct FrameDeleter {
  void operator()(uvc_frame_t *f) const { if (f) uvc_free_frame(f); }
};
using FramePtr = std::unique_ptr<uvc_frame_t, FrameDeleter>;

/** Allocate a library-owned frame of exactly @p bytes, so a redzone sits
 * right after the data and a one-byte overread is caught.
 *
 * The contents only need to be deterministic and non-uniform: these frames
 * drive the bounds-checking tests, and what the converters compute from them
 * is checked against the reference images instead. */
FramePtr MakeFrame(uvc_frame_format fmt, uint32_t width, uint32_t height,
                   size_t bytes) {
  FramePtr f(uvc_allocate_frame(bytes));
  if (!f) return f;

  f->width = width;
  f->height = height;
  f->frame_format = fmt;
  f->step = 0;

  auto *p = static_cast<uint8_t *>(f->data);
  for (size_t i = 0; i < bytes; ++i)
    p[i] = static_cast<uint8_t>(i * 7 + 13);

  return f;
}

/** An output frame whose buffer the caller owns. uvc_ensure_frame_size()
 * only rejects a too-small caller buffer, leaving data_bytes at the caller's
 * larger value, so a bound taken from it overshoots the geometry. */
struct CallerOwnedFrame {
  uvc_frame_t frame{};
  std::vector<uint8_t> storage;

  CallerOwnedFrame(uvc_frame_format fmt, size_t bytes) : storage(bytes, 0) {
    frame.data = storage.data();
    frame.data_bytes = bytes;
    frame.library_owns_data = 0;
    frame.frame_format = fmt;
  }
};

/** The converters under test, named so failures identify themselves. */
struct Converter {
  const char *name;
  uvc_error_t (*fn)(uvc_frame_t *, uvc_frame_t *);
  uvc_frame_format in_format;
  /** Output bytes per pixel, for sizing the destination. */
  int out_bpp;
};

const Converter kConverters[] = {
  { "uvc_yuyv2rgb", uvc_yuyv2rgb, UVC_FRAME_FORMAT_YUYV, 3 },
  { "uvc_yuyv2bgr", uvc_yuyv2bgr, UVC_FRAME_FORMAT_YUYV, 3 },
  { "uvc_yuyv2y",   uvc_yuyv2y,   UVC_FRAME_FORMAT_YUYV, 1 },
  { "uvc_yuyv2uv",  uvc_yuyv2uv,  UVC_FRAME_FORMAT_YUYV, 1 },
  { "uvc_uyvy2rgb", uvc_uyvy2rgb, UVC_FRAME_FORMAT_UYVY, 3 },
  { "uvc_uyvy2bgr", uvc_uyvy2bgr, UVC_FRAME_FORMAT_UYVY, 3 },
};

struct Geometry {
  uint32_t width, height;
};

/* Resolutions taken from the descriptor dumps in cameras/. */
const Geometry kGeometries[] = {
  {160, 120}, {320, 240}, {640, 480}, {1280, 720},
};

std::ostream &operator<<(std::ostream &os, const Geometry &g) {
  return os << g.width << "x" << g.height;
}

/* ------------------------------------------------------- converter matrix */

class ConverterTest
    : public ::testing::TestWithParam<std::tuple<Converter, Geometry>> {
 protected:
  const Converter &conv() const { return std::get<0>(GetParam()); }
  const Geometry &geom() const { return std::get<1>(GetParam()); }

  size_t InBytes() const {
    return static_cast<size_t>(geom().width) * geom().height * 2;
  }
  size_t OutBytes() const {
    return static_cast<size_t>(geom().width) * geom().height * conv().out_bpp;
  }
};

std::string ParamName(
    const ::testing::TestParamInfo<ConverterTest::ParamType> &info) {
  const auto &c = std::get<0>(info.param);
  const auto &g = std::get<1>(info.param);
  return std::string(c.name) + "_" + std::to_string(g.width) + "x" +
         std::to_string(g.height);
}

/** Baseline: the hardening must not start rejecting valid input. */
TEST_P(ConverterTest, ValidFrameSucceeds) {
  const Converter c = conv();
  const Geometry g = geom();
  const size_t in_bytes = InBytes();

  EXPECT_MEMORY_SAFE({
    auto in = MakeFrame(c.in_format, g.width, g.height, in_bytes);
    REQUIRE_IN_CHILD(in != nullptr);
    FramePtr out(uvc_allocate_frame(0));
    REQUIRE_IN_CHILD(out != nullptr);

    REQUIRE_IN_CHILD(c.fn(in.get(), out.get()) == UVC_SUCCESS);
    REQUIRE_IN_CHILD(out->width == g.width);
    REQUIRE_IN_CHILD(out->height == g.height);
  }) << c.name << " on a valid " << g << " frame";
}

/* Regression test for libuvc/libuvc#211: an input one byte short of what
 * width * height implies, as a truncated USB transfer produces. */
TEST_P(ConverterTest, ShortInputRejected) {
  const size_t full = InBytes();

  const Converter c = conv();
  const Geometry g = geom();

  EXPECT_MEMORY_SAFE({
    auto in = MakeFrame(c.in_format, g.width, g.height, full - 1);
    REQUIRE_IN_CHILD(in != nullptr);
    FramePtr out(uvc_allocate_frame(0));
    REQUIRE_IN_CHILD(out != nullptr);
    REQUIRE_IN_CHILD(c.fn(in.get(), out.get()) != UVC_SUCCESS);
  }) << c.name << " on a frame " << (full - 1) << " bytes long for a "
     << g << " image needing " << full;
}

/** A drastically truncated frame. */
TEST_P(ConverterTest, SingleByteInputRejected) {
  const Converter c = conv();
  const Geometry g = geom();

  EXPECT_MEMORY_SAFE({
    auto in = MakeFrame(c.in_format, g.width, g.height, 1);
    REQUIRE_IN_CHILD(in != nullptr);
    FramePtr out(uvc_allocate_frame(0));
    REQUIRE_IN_CHILD(out != nullptr);
    REQUIRE_IN_CHILD(c.fn(in.get(), out.get()) != UVC_SUCCESS);
  }) << c.name;
}

/** An empty frame must be rejected, not dereferenced. */
TEST_P(ConverterTest, EmptyInputRejected) {
  const Converter c = conv();
  const Geometry g = geom();

  EXPECT_MEMORY_SAFE({
    auto in = MakeFrame(c.in_format, g.width, g.height, 0);
    REQUIRE_IN_CHILD(in != nullptr);
    FramePtr out(uvc_allocate_frame(0));
    REQUIRE_IN_CHILD(out != nullptr);
    REQUIRE_IN_CHILD(c.fn(in.get(), out.get()) != UVC_SUCCESS);
  }) << c.name;
}

/* An oversized caller-owned output must not make the converter read past a
 * correctly sized input. */
TEST_P(ConverterTest, OversizedCallerOwnedOutputDoesNotOverreadInput) {
  const Converter c = conv();
  const Geometry g = geom();
  const size_t in_bytes = InBytes();
  const size_t out_bytes = OutBytes();

  EXPECT_MEMORY_SAFE({
    auto in = MakeFrame(c.in_format, g.width, g.height, in_bytes);
    REQUIRE_IN_CHILD(in != nullptr);

    /* Well over what the frame needs, so a data_bytes bound overshoots. */
    CallerOwnedFrame out(UVC_FRAME_FORMAT_RGB, out_bytes * 4 + 3 * 8);

    REQUIRE_IN_CHILD(c.fn(in.get(), &out.frame) == UVC_SUCCESS);
  }) << c.name;
}

/** An input frame whose format does not match must be rejected. */
TEST_P(ConverterTest, WrongInputFormatRejected) {
  auto in = MakeFrame(UVC_FRAME_FORMAT_GRAY8, geom().width, geom().height,
                      InBytes());
  ASSERT_NE(in, nullptr);
  FramePtr out(uvc_allocate_frame(0));
  ASSERT_NE(out, nullptr);

  EXPECT_EQ(conv().fn(in.get(), out.get()), UVC_ERROR_INVALID_PARAM);
}

/** A too-small caller-owned output must be refused, not written past. */
TEST_P(ConverterTest, UndersizedCallerOwnedOutputRejected) {
  auto in = MakeFrame(conv().in_format, geom().width, geom().height, InBytes());
  ASSERT_NE(in, nullptr);

  CallerOwnedFrame out(UVC_FRAME_FORMAT_RGB, OutBytes() - 1);

  EXPECT_NE(conv().fn(in.get(), &out.frame), UVC_SUCCESS);
}

INSTANTIATE_TEST_SUITE_P(
    Converters, ConverterTest,
    ::testing::Combine(::testing::ValuesIn(kConverters),
                       ::testing::ValuesIn(kGeometries)),
    ParamName);

/** An unsupported format must be refused, not passed to a converter. */
TEST(FrameConversion, AnyToRgbRejectsUnsupportedFormat) {
  EXPECT_MEMORY_SAFE({
    auto in = MakeFrame(UVC_FRAME_FORMAT_UNKNOWN, 32, 32, 32 * 32 * 2);
    REQUIRE_IN_CHILD(in != nullptr);
    FramePtr out(uvc_allocate_frame(0));
    REQUIRE_IN_CHILD(out != nullptr);
    REQUIRE_IN_CHILD(uvc_any2rgb(in.get(), out.get()) != UVC_SUCCESS);
    REQUIRE_IN_CHILD(uvc_any2bgr(in.get(), out.get()) != UVC_SUCCESS);
  });
}

/** A short frame must not make uvc_duplicate_frame read the geometry's
 * worth instead of data_bytes. */
TEST(FrameConversion, DuplicateShortFrameStaysInBounds) {
  EXPECT_MEMORY_SAFE({
    auto in = MakeFrame(UVC_FRAME_FORMAT_YUYV, 64, 32, 64);  /* far too short */
    REQUIRE_IN_CHILD(in != nullptr);
    FramePtr out(uvc_allocate_frame(0));
    REQUIRE_IN_CHILD(out != nullptr);
    (void)uvc_duplicate_frame(in.get(), out.get());
  });
}

}  // namespace
