/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (C) 2014 Robert Xiao
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the author nor other contributors may be
*     used to endorse or promote products derived from this software
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/**
 * @defgroup frame Frame processing
 */
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"
#include <jpeglib.h>
#include <setjmp.h>

extern uvc_error_t uvc_ensure_frame_size(uvc_frame_t *frame, size_t need_bytes);

struct error_mgr {
  struct jpeg_error_mgr super;
  jmp_buf jmp;
};

static void _error_exit(j_common_ptr cinfo) {
  struct error_mgr *myerr = (struct error_mgr *)cinfo->err;
  (*cinfo->err->output_message)(cinfo);
  longjmp(myerr->jmp, 1);
}

/** @brief Convert an MJPEG frame to RGB
 * @ingroup frame
 *
 * @param in MJPEG frame
 * @param out RGB frame
 */
uvc_error_t uvc_mjpeg2rgb(uvc_frame_t *in, uvc_frame_t *out) {
  struct jpeg_decompress_struct cinfo;
  struct error_mgr jerr;
  size_t lines_read;

  if (in->color_format != UVC_COLOR_FORMAT_MJPEG)
    return UVC_ERROR_INVALID_PARAM;

  if (uvc_ensure_frame_size(out, in->width * in->height * 3) < 0)
    return UVC_ERROR_NO_MEM;

  out->width = in->width;
  out->height = in->height;
  out->color_format = UVC_COLOR_FORMAT_RGB;
  out->step = in->width * 3;
  out->sequence = in->sequence;
  out->capture_time = in->capture_time;
  out->source = in->source;

  cinfo.err = jpeg_std_error(&jerr.super);
  jerr.super.error_exit = _error_exit;

  if(setjmp(jerr.jmp)) {
    goto fail;
  }

  /* @todo This doesn't actually read MJPEG; it only reads regular JPEG.
  To support MJPEG, you would have to initialize the JPEG decoder using
  the MJPEG default Huffman tables.

  See http://www.ffmpeg.org/ffmpeg-bitstream-filters.html#mjpeg2jpeg for
  more info. The tables needed are said to be in the OpenDML spec. */
  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, in->data, in->data_bytes);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  lines_read = 0;
  while (cinfo.output_scanline < cinfo.output_height) {
    void *buffer[1] = { out->data + lines_read * out->step };
    int num_scanlines;

    num_scanlines = jpeg_read_scanlines(&cinfo, buffer, 1);
    lines_read += num_scanlines;
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  return 0;

fail:
  jpeg_destroy_decompress(&cinfo);
  return UVC_ERROR_OTHER;
}
