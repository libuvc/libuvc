#ifndef CONVERT_H
#define CONVERT_H

#include <stddef.h>

#include <libuvc/libuvc.h>

/* Whether convert_frame() can handle frames of this format. */
int convert_supported(enum uvc_frame_format format);

/* Write a frame into an RGBX_8888 buffer with the frame's width and height,
 * dst_stride bytes apart. Returns 0 on success, or a negative value if the
 * frame is truncated, corrupt or of an unsupported format. */
int convert_frame(const uvc_frame_t *frame, void *dst, size_t dst_stride);

#endif
