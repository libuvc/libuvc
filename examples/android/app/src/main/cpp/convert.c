/*
 * Conversion of the frame formats libuvc knows about to the RGBX_8888
 * buffers of an ANativeWindow.
 *
 * Uncompressed formats are converted here, in one pass straight into the
 * window buffer. MJPEG is decoded with the platform's AImageDecoder (API 30),
 * so the app needs no JPEG library of its own and libuvc is built with
 * DISABLE_JPEG. H.264 and the other frame-based formats would need MediaCodec
 * and are not handled.
 */
#include "convert.h"

#include <stdint.h>

#include <android/imagedecoder.h>

static inline uint8_t clamp8(int v) {
  return v < 0 ? 0 : v > 255 ? 255 : v;
}

static inline void put_rgb(uint8_t *dst, int r, int g, int b) {
  dst[0] = r;
  dst[1] = g;
  dst[2] = b;
  dst[3] = 0xff;
}

/* ITU-R BT.601, limited range: SMPTE 170M is the UVC default matrix. */
static inline void put_yuv(uint8_t *dst, int y, int u, int v) {
  int c = 298 * (y - 16) + 128;

  u -= 128;
  v -= 128;
  put_rgb(dst,
          clamp8((c + 409 * v) >> 8),
          clamp8((c - 100 * u - 208 * v) >> 8),
          clamp8((c + 516 * u) >> 8));
}

/* YUYV and UYVY: two pixels in four bytes. off[] gives the positions of Y0,
 * U, Y1 and V within them. */
static void convert_422(const uint8_t *src, uint8_t *dst, size_t dst_stride,
                        int width, int height, const int off[4]) {
  for (int row = 0; row < height; row++, dst += dst_stride) {
    const uint8_t *s = src + (size_t) row * width * 2;
    uint8_t *d = dst;

    for (int x = 0; x + 1 < width; x += 2, s += 4, d += 8) {
      put_yuv(d, s[off[0]], s[off[1]], s[off[3]]);
      put_yuv(d + 4, s[off[2]], s[off[1]], s[off[3]]);
    }
  }
}

/* 4:2:0: a full-size Y plane followed by quarter-size chroma, either
 * interleaved (NV12, NV21, P010) or as a U plane then a V plane (I420).
 * Samples are bps bytes wide, little-endian, and only their most significant
 * byte is used. */
static void convert_420(const uint8_t *src, uint8_t *dst, size_t dst_stride,
                        int width, int height, int bps,
                        int interleaved, int v_first) {
  const size_t cw = (width + 1) / 2, ch = (height + 1) / 2;
  const uint8_t *y_plane = src + bps - 1;
  const uint8_t *c_plane = y_plane + (size_t) width * height * bps;
  const uint8_t *u_plane, *v_plane;
  size_t c_step, c_row;

  if (interleaved) {
    c_step = 2 * bps;
    c_row = cw * c_step;
    u_plane = c_plane + (v_first ? bps : 0);
    v_plane = c_plane + (v_first ? 0 : bps);
  } else {
    c_step = bps;
    c_row = cw * c_step;
    u_plane = c_plane;
    v_plane = c_plane + cw * ch * bps;
  }

  for (int row = 0; row < height; row++, dst += dst_stride) {
    const uint8_t *ys = y_plane + (size_t) row * width * bps;
    const uint8_t *us = u_plane + (row / 2) * c_row;
    const uint8_t *vs = v_plane + (row / 2) * c_row;

    for (int x = 0; x < width; x++)
      put_yuv(dst + 4 * x, ys[x * bps], us[(x / 2) * c_step],
              vs[(x / 2) * c_step]);
  }
}

static void convert_gray8(const uint8_t *src, uint8_t *dst, size_t dst_stride,
                          int width, int height) {
  for (int row = 0; row < height; row++, dst += dst_stride) {
    const uint8_t *s = src + (size_t) row * width;

    for (int x = 0; x < width; x++)
      put_rgb(dst + 4 * x, s[x], s[x], s[x]);
  }
}

/* Y16 cameras are mostly thermal or depth sensors, whose values only cover a
 * small part of the 16-bit range: stretch each frame's own range to 0..255,
 * or the picture would be uniformly black. */
static void convert_gray16(const uint8_t *src, uint8_t *dst, size_t dst_stride,
                           int width, int height) {
  const size_t pixels = (size_t) width * height;
  unsigned min = 0xffff, max = 0;

  for (size_t i = 0; i < pixels; i++) {
    unsigned v = src[2 * i] | src[2 * i + 1] << 8;

    if (v < min)
      min = v;
    if (v > max)
      max = v;
  }

  for (int row = 0; row < height; row++, dst += dst_stride) {
    const uint8_t *s = src + (size_t) row * width * 2;

    for (int x = 0; x < width; x++) {
      unsigned v = s[2 * x] | s[2 * x + 1] << 8;
      int g = max > min ? (v - min) * 255 / (max - min) : 0;

      put_rgb(dst + 4 * x, g, g, g);
    }
  }
}

static void convert_rgb24(const uint8_t *src, uint8_t *dst, size_t dst_stride,
                          int width, int height, int bgr) {
  for (int row = 0; row < height; row++, dst += dst_stride) {
    const uint8_t *s = src + (size_t) row * width * 3;

    for (int x = 0; x < width; x++, s += 3) {
      if (bgr)
        put_rgb(dst + 4 * x, s[2], s[1], s[0]);
      else
        put_rgb(dst + 4 * x, s[0], s[1], s[2]);
    }
  }
}

/* UVC cameras usually leave the Huffman tables out of their MJPEG frames.
 * libuvc's own decoder adds the standard ones; the platform's libjpeg-turbo
 * falls back on them by itself. */
static int decode_mjpeg(const uvc_frame_t *frame, uint8_t *dst,
                        size_t dst_stride) {
  if (__builtin_available(android 30, *)) {
    AImageDecoder *decoder;
    const AImageDecoderHeaderInfo *info;
    int ret;

    ret = AImageDecoder_createFromBuffer(frame->data, frame->data_bytes,
                                         &decoder);
    if (ret != ANDROID_IMAGE_DECODER_SUCCESS)
      return ret;

    /* The window buffer was sized for the negotiated mode: do not let a
     * frame of another size overrun it. */
    info = AImageDecoder_getHeaderInfo(decoder);
    if (AImageDecoderHeaderInfo_getWidth(info) != (int32_t) frame->width ||
        AImageDecoderHeaderInfo_getHeight(info) != (int32_t) frame->height)
      ret = ANDROID_IMAGE_DECODER_BAD_PARAMETER;
    else
      ret = AImageDecoder_decodeImage(decoder, dst, dst_stride,
                                      dst_stride * frame->height);

    AImageDecoder_delete(decoder);
    return ret;
  }

  return -1;
}

int convert_supported(enum uvc_frame_format format) {
  switch (format) {
  case UVC_FRAME_FORMAT_MJPEG:
    if (__builtin_available(android 30, *))
      return 1;
    return 0;
  case UVC_FRAME_FORMAT_YUYV:
  case UVC_FRAME_FORMAT_UYVY:
  case UVC_FRAME_FORMAT_NV12:
  case UVC_FRAME_FORMAT_NV21:
  case UVC_FRAME_FORMAT_I420:
  case UVC_FRAME_FORMAT_P010:
  case UVC_FRAME_FORMAT_GRAY8:
  case UVC_FRAME_FORMAT_GRAY16:
  case UVC_FRAME_FORMAT_RGB:
  case UVC_FRAME_FORMAT_BGR:
  case UVC_FRAME_FORMAT_BY8:
  case UVC_FRAME_FORMAT_BA81:
  case UVC_FRAME_FORMAT_SGRBG8:
  case UVC_FRAME_FORMAT_SGBRG8:
  case UVC_FRAME_FORMAT_SRGGB8:
  case UVC_FRAME_FORMAT_SBGGR8:
    return 1;
  default:
    return 0;
  }
}

int convert_frame(const uvc_frame_t *frame, void *dst, size_t dst_stride) {
  static const int yuyv[4] = { 0, 1, 2, 3 }, uyvy[4] = { 1, 0, 3, 2 };
  const uint8_t *src = frame->data;
  const int width = frame->width, height = frame->height;
  const size_t pixels = (size_t) width * height;
  const size_t chroma = 2 * (size_t) ((width + 1) / 2) * ((height + 1) / 2);

  /* A frame cut short by a lost packet is delivered all the same: check that
   * it holds a whole picture before reading it. */
  switch (frame->frame_format) {
  case UVC_FRAME_FORMAT_MJPEG:
    return decode_mjpeg(frame, dst, dst_stride);
  case UVC_FRAME_FORMAT_YUYV:
  case UVC_FRAME_FORMAT_UYVY:
    if (frame->data_bytes < 2 * pixels)
      return -1;
    convert_422(src, dst, dst_stride, width, height,
                frame->frame_format == UVC_FRAME_FORMAT_YUYV ? yuyv : uyvy);
    return 0;
  case UVC_FRAME_FORMAT_NV12:
  case UVC_FRAME_FORMAT_NV21:
  case UVC_FRAME_FORMAT_I420:
    if (frame->data_bytes < pixels + chroma)
      return -1;
    convert_420(src, dst, dst_stride, width, height, 1,
                frame->frame_format != UVC_FRAME_FORMAT_I420,
                frame->frame_format == UVC_FRAME_FORMAT_NV21);
    return 0;
  case UVC_FRAME_FORMAT_P010:
    if (frame->data_bytes < 2 * (pixels + chroma))
      return -1;
    convert_420(src, dst, dst_stride, width, height, 2, 1, 0);
    return 0;
  case UVC_FRAME_FORMAT_GRAY8:
  /* Raw Bayer mosaics are shown as they are, in grey. */
  case UVC_FRAME_FORMAT_BY8:
  case UVC_FRAME_FORMAT_BA81:
  case UVC_FRAME_FORMAT_SGRBG8:
  case UVC_FRAME_FORMAT_SGBRG8:
  case UVC_FRAME_FORMAT_SRGGB8:
  case UVC_FRAME_FORMAT_SBGGR8:
    if (frame->data_bytes < pixels)
      return -1;
    convert_gray8(src, dst, dst_stride, width, height);
    return 0;
  case UVC_FRAME_FORMAT_GRAY16:
    if (frame->data_bytes < 2 * pixels)
      return -1;
    convert_gray16(src, dst, dst_stride, width, height);
    return 0;
  case UVC_FRAME_FORMAT_RGB:
  case UVC_FRAME_FORMAT_BGR:
    if (frame->data_bytes < 3 * pixels)
      return -1;
    convert_rgb24(src, dst, dst_stride, width, height,
                  frame->frame_format == UVC_FRAME_FORMAT_BGR);
    return 0;
  default:
    return -1;
  }
}
