/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (C) 2010-2012 Ken Tossell
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
 * @defgroup streaming Streaming control functions
 */

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

uvc_frame_desc_t *uvc_find_frame_desc(uvc_device_handle_t *devh,
    uint16_t format_id, uint16_t frame_id);
void *_uvc_user_caller(void *arg);
void _uvc_populate_frame(uvc_device_handle_t *devh);

struct format_table_entry {
  enum uvc_color_format format;
  uint8_t abstract_fmt;
  uint8_t guid[16];
  enum uvc_color_format *children;
};

static enum uvc_color_format UVC_COLOR_FORMAT_UNCOMPRESSED_children[] = {
  UVC_COLOR_FORMAT_YUYV, UVC_COLOR_FORMAT_UYVY, UVC_COLOR_FORMAT_GRAY8, 0
};

static enum uvc_color_format UVC_COLOR_FORMAT_YUYV_children[] = {
  0
};

static enum uvc_color_format UVC_COLOR_FORMAT_UYVY_children[] = {
  0
};

static enum uvc_color_format UVC_COLOR_FORMAT_GRAY8_children[] = {
  0
};

#define FMT(_fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
  {.format = _fmt, \
   .abstract_fmt = 0, \
   .guid = {a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p}, \
   .children = _fmt ## _children}

#define ABS_FMT(_fmt) \
  {.format = _fmt, \
   .abstract_fmt = 1, \
   .guid = {}, \
   .children = _fmt ## _children}

static struct format_table_entry _format_table[] = {
  ABS_FMT(UVC_COLOR_FORMAT_UNCOMPRESSED),
  FMT(UVC_COLOR_FORMAT_YUYV,
    'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71),
  FMT(UVC_COLOR_FORMAT_UYVY,
    'U',  'Y',  'V',  'Y', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71),
  FMT(UVC_COLOR_FORMAT_GRAY8,
    'Y',  '8',  '0',  '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71),
};

static uint8_t _uvc_color_format_matches_guid(enum uvc_color_format fmt, uint8_t guid[16]) {
  int format_idx;
  int child_idx;

  for (format_idx = 0; format_idx < sizeof(_format_table)/sizeof(_format_table[0]); ++format_idx) {

    if (_format_table[format_idx].format == fmt) {
      if (!_format_table[format_idx].abstract_fmt && !memcmp(guid, _format_table[format_idx].guid, 16))
        return 1;

      for (child_idx = 0; _format_table[format_idx].children[child_idx] != 0; ++child_idx) {
        if (_uvc_color_format_matches_guid(_format_table[format_idx].children[child_idx], guid))
          return 1;
      }

      return 0;
    }
  }

  return 0;
}

static enum uvc_color_format uvc_color_format_for_guid(uint8_t guid[16]) {
  int format_idx;

  for (format_idx = 0; format_idx < sizeof(_format_table)/sizeof(_format_table[0]); ++format_idx) {
    if (!memcmp(_format_table[format_idx].guid, guid, 16))
      return _format_table[format_idx].format;
  }

  return UVC_COLOR_FORMAT_UNKNOWN;
}

/** @internal
 * Run a streaming control query
 * @param[in] devh UVC device
 * @param[in,out] ctrl Control block
 * @param[in] probe Whether this is a probe query or a commit query
 * @param[in] req Query type
 */
uvc_error_t uvc_query_stream_ctrl(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uint8_t probe,
    enum uvc_req_code req) {
  uint8_t buf[34];
  size_t len;
  uvc_error_t err;

  bzero(buf, sizeof(buf));

  if (devh->info->ctrl_if.bcdUVC >= 0x0110)
    len = 34;
  else
    len = 26;

  /* prepare for a SET transfer */
  if (req == UVC_SET_CUR) {
    SHORT_TO_SW(ctrl->bmHint, buf);
    buf[2] = ctrl->bFormatIndex;
    buf[3] = ctrl->bFrameIndex;
    INT_TO_DW(ctrl->dwFrameInterval, buf + 4);
    SHORT_TO_SW(ctrl->wKeyFrameRate, buf + 8);
    SHORT_TO_SW(ctrl->wPFrameRate, buf + 10);
    SHORT_TO_SW(ctrl->wCompQuality, buf + 12);
    SHORT_TO_SW(ctrl->wCompWindowSize, buf + 14);
    SHORT_TO_SW(ctrl->wDelay, buf + 16);
    INT_TO_DW(ctrl->dwMaxVideoFrameSize, buf + 18);
    INT_TO_DW(ctrl->dwMaxPayloadTransferSize, buf + 22);

    if (len == 34) {
      /** @todo support UVC 1.1 */
      return UVC_ERROR_NOT_SUPPORTED;
    }
  }

  /* do the transfer */
  err = libusb_control_transfer(
      devh->usb_devh,
      req == UVC_SET_CUR ? 0x21 : 0xA1,
      req,
      probe ? (UVC_VS_PROBE_CONTROL << 8) : (UVC_VS_COMMIT_CONTROL << 8),
      devh->info->stream_ifs->bInterfaceNumber, /** @todo support multiple stream ifs */
      buf, len, 0
  );

  if (err <= 0) {
    return err;
  }

  /* now decode following a GET transfer */
  if (req != UVC_SET_CUR) {
    ctrl->bmHint = SW_TO_SHORT(buf);
    ctrl->bFormatIndex = buf[2];
    ctrl->bFrameIndex = buf[3];
    ctrl->dwFrameInterval = DW_TO_INT(buf + 4);
    ctrl->wKeyFrameRate = SW_TO_SHORT(buf + 8);
    ctrl->wPFrameRate = SW_TO_SHORT(buf + 10);
    ctrl->wCompQuality = SW_TO_SHORT(buf + 12);
    ctrl->wCompWindowSize = SW_TO_SHORT(buf + 14);
    ctrl->wDelay = SW_TO_SHORT(buf + 16);
    ctrl->dwMaxVideoFrameSize = DW_TO_INT(buf + 18);
    ctrl->dwMaxPayloadTransferSize = DW_TO_INT(buf + 22);

    if (len == 34) {
      /** @todo support UVC 1.1 */
      return UVC_ERROR_NOT_SUPPORTED;
    }

    /* fix up block for cameras that fail to set dwMax* */
    if (ctrl->dwMaxVideoFrameSize == 0) {
      uvc_frame_desc_t *frame = uvc_find_frame_desc(devh, ctrl->bFormatIndex, ctrl->bFrameIndex);

      if (frame) {
        ctrl->dwMaxVideoFrameSize = frame->dwMaxVideoFrameBufferSize;
      }
    }
  }

  return UVC_SUCCESS;
}

/** @internal
 * @brief Find the descriptor for a specific frame configuration
 * @param devh UVC device
 * @param format_id Index of format class descriptor
 * @param frame_id Index of frame descriptor
 */
uvc_frame_desc_t *uvc_find_frame_desc(uvc_device_handle_t *devh,
    uint16_t format_id, uint16_t frame_id) {
  uvc_format_desc_t *format;
  uvc_frame_desc_t *frame;

  DL_FOREACH(devh->info->stream_ifs->format_descs, format)
    if (format->bFormatIndex == format_id)
      break;

  if (format)
    DL_FOREACH(format->frame_descs, frame)
      if (frame->bFrameIndex == frame_id)
        break;

  if (frame)
    return frame;
}

/** @internal
 * @brief Get the UVC format type for a libuvc color coding
 */
uint8_t uvc_get_desc_subtype_for_color_format(
  	enum uvc_color_format cf, enum uvc_vs_desc_subtype *fc
    ) {
  switch (cf) {
    case UVC_COLOR_FORMAT_YUYV:
      *fc = UVC_VS_FORMAT_UNCOMPRESSED;
      break;
    case UVC_COLOR_FORMAT_MJPEG:
      *fc = UVC_VS_FORMAT_MJPEG;
      break;
    default:
      return 0;
  }
  
  return 1;
}

/** Get a negotiated streaming control block for some common parameters.
 * @ingroup streaming
 *
 * @param[in] devh Device handle
 * @param[in,out] ctrl Control block
 * @param[in] format_class Type of streaming format
 * @param[in] width Desired frame width
 * @param[in] height Desired frame height
 * @param[in] fps Frame rate, frames per second
 */
uvc_error_t uvc_get_stream_ctrl_format_size(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    enum uvc_color_format cf,
    int width, int height,
    int fps) {	
  uvc_streaming_interface_t *stream_if;
  uint8_t found_frame;
  enum uvc_vs_desc_subtype format_class;

  /* get the max values */
  uvc_query_stream_ctrl(
      devh, ctrl, 1, UVC_GET_MAX
  );

  found_frame = 0;

  /* find a matching frame descriptor and interval */
  DL_FOREACH(devh->info->stream_ifs, stream_if) {
    uvc_format_desc_t *format;

    DL_FOREACH(stream_if->format_descs, format) {
      uvc_frame_desc_t *frame;

      if (!_uvc_color_format_matches_guid(cf, format->guidFormat))
        continue;

      DL_FOREACH(format->frame_descs, frame) {
        if (frame->wWidth != width || frame->wHeight != height)
          continue;

        uint32_t *interval;

        if (frame->intervals) {
          for (interval = frame->intervals; *interval && !found_frame; ++interval) {
            if (10000000 / *interval == (unsigned int) fps) {
              ctrl->bmHint = (1 << 0); /* don't negotiate interval */
              ctrl->bFormatIndex = format->bFormatIndex;
              ctrl->bFrameIndex = frame->bFrameIndex;
              ctrl->dwFrameInterval = *interval;

              found_frame = 1;
            }
          }
        } else {
          uint32_t interval_100ns = 10000000 / fps;
          uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;

          if (interval_100ns >= frame->dwMinFrameInterval
              && interval_100ns <= frame->dwMaxFrameInterval
              && !(interval_offset
                   && (interval_offset % frame->dwFrameIntervalStep))) {
            ctrl->bmHint = (1 << 0);
            ctrl->bFormatIndex = format->bFormatIndex;
            ctrl->bFrameIndex = frame->bFrameIndex;
            ctrl->dwFrameInterval = interval_100ns;

            found_frame = 1;
          }
        }

        if (found_frame) break;
      }

      if (found_frame) break;
    }

    if (found_frame) break;
  }

  if (!found_frame) {
    return UVC_ERROR_INVALID_MODE;
  }

  return uvc_probe_stream_ctrl(devh, ctrl);
}

/** @internal
 * Negotiate streaming parameters with the device
 *
 * @param[in] devh UVC device
 * @param[in,out] ctrl Control block
 */
uvc_error_t uvc_probe_stream_ctrl(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl) {
 
  uvc_query_stream_ctrl(
      devh, ctrl, 1, UVC_SET_CUR
  );

  uvc_query_stream_ctrl(
      devh, ctrl, 1, UVC_GET_CUR
  );

  /** @todo make sure that worked */
  return UVC_SUCCESS;
}

/** @internal
 * @brief Isochronous transfer callback
 * 
 * Processes stream, places frames into buffer, signals listeners
 * (such as user callback thread and any polling thread) on new frame
 *
 * @param transfer Active transfer
 */ 
void _uvc_iso_callback(struct libusb_transfer *transfer) {
  uvc_device_handle_t *devh;
  int packet_id;

  /* per packet */
  uint8_t *pktbuf;
  uint8_t check_header;
  size_t header_len;
  struct libusb_iso_packet_descriptor *pkt;
  uint8_t *tmp_buf;

  static uint8_t isight_tag[] = {
    0x11, 0x22, 0x33, 0x44,
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xfa, 0xce
  };

  devh = transfer->user_data;

  switch (transfer->status) {
  case LIBUSB_TRANSFER_COMPLETED:
    for (packet_id = 0; packet_id < transfer->num_iso_packets; ++packet_id) {
      check_header = 1;

      pkt = transfer->iso_packet_desc + packet_id;

      if (pkt->status != 0) {
        printf("bad packet (transfer): %d\n", pkt->status);
        continue;
      }

      if (pkt->actual_length == 0) {
        continue;
      }

      pktbuf = libusb_get_iso_packet_buffer_simple(transfer, packet_id);

      if (devh->is_isight) {
        if (pkt->actual_length < 30 ||
            (memcmp(isight_tag, pktbuf + 2, sizeof(isight_tag))
             && memcmp(isight_tag, pktbuf + 3, sizeof(isight_tag)))) {
          check_header = 0;
          header_len = 0;
        } else {
          header_len = pktbuf[0];
        }
      } else {
        header_len = pktbuf[0];
      }

      if (check_header && pktbuf[1] & 0x40) {
        printf("bad packet\n");
        continue;
      }

      /** @todo support sending the frame on EOF instead of on flip(FID) */
      if (check_header && devh->stream.fid != (pktbuf[1] & 1)) {
        /* printf("frame %d LEN: %u\n", devh->stream.seq, devh->stream.got_bytes); */

#if 0
	char filename[64];
	FILE *fp;
	sprintf(filename, "frame-%d.bin", devh->stream.seq);
	fp = fopen(filename, "w");
	fwrite(devh->stream.outbuf, 1, devh->stream.got_bytes, fp);
	fclose(fp);
#endif

        pthread_mutex_lock(&devh->stream.cb_mutex);

        /* swap the buffers */
	tmp_buf = devh->stream.holdbuf;
	devh->stream.hold_bytes = devh->stream.got_bytes;
	devh->stream.holdbuf = devh->stream.outbuf;
	devh->stream.outbuf = tmp_buf;
	devh->stream.hold_last_scr = devh->stream.last_scr;
	devh->stream.hold_pts = devh->stream.pts;
	devh->stream.hold_seq = devh->stream.seq;

        pthread_cond_signal(&devh->stream.cb_cond);
        pthread_mutex_unlock(&devh->stream.cb_mutex);

        devh->stream.seq++;
        devh->stream.got_bytes = 0;
        devh->stream.last_scr = 0;
        devh->stream.pts = 0;
        devh->stream.fid = pktbuf[1] & 1;
      }

      if (check_header) {
        if (pktbuf[1] & (1 << 2))
          devh->stream.pts = DW_TO_INT(pktbuf + 2);

        if (pktbuf[1] & (1 << 3))
          devh->stream.last_scr = DW_TO_INT(pktbuf + 6);

        if (devh->is_isight)
          continue; // don't look for data after an iSight header
      }

      if (pkt->actual_length - header_len > 0)
        memcpy(devh->stream.outbuf + devh->stream.got_bytes, pktbuf + header_len, pkt->actual_length - header_len);

      devh->stream.got_bytes += pkt->actual_length - header_len;
    }
    break;
  case LIBUSB_TRANSFER_CANCELLED: 
    pthread_mutex_lock(&devh->stream.cb_mutex);
    pthread_cond_signal(&devh->stream.cb_cond);
    pthread_mutex_unlock(&devh->stream.cb_mutex);

    free(transfer->buffer);
    libusb_free_transfer(transfer);
    break;
  }
  
  if (!devh->stream.stop)
    libusb_submit_transfer(transfer);
}

/** Begin streaming video from the camera into the callback function.
 * @ingroup streaming
 *
 * @param devh UVC device
 * @param ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 * @param isochronous Whether to use isochronous transfers rather than bulk ones
 */
uvc_error_t uvc_start_streaming(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uvc_frame_callback_t *cb,
    void *user_ptr,
    uint8_t isochronous
) {
  /* USB interface we'll be using */
  const struct libusb_interface *interface;
  int interface_id;

  /* Chosen frame and format descriptors */
  uvc_frame_desc_t *frame_desc;
  uvc_format_desc_t *format_desc;

  uvc_error_t ret;

  if (devh->streaming)
    return UVC_ERROR_BUSY;

  ret = uvc_query_stream_ctrl(
      devh, ctrl, 0, UVC_SET_CUR
  );

  if (ret != UVC_SUCCESS)
    return ret;

  devh->cur_ctrl = *ctrl;

  frame_desc = uvc_find_frame_desc(devh, ctrl->bFormatIndex, ctrl->bFrameIndex);
  format_desc = frame_desc->parent;

  if (format_desc->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED) {
    devh->stream.color_format = uvc_color_format_for_guid(format_desc->guidFormat);
  } else {
    return UVC_ERROR_NOT_SUPPORTED;
  }

  // Get the interface that provides the chosen format and frame configuration
  interface_id = format_desc->parent->bInterfaceNumber;
  interface = &devh->info->config->interface[interface_id];

  // Set up the streaming status and data space
  devh->stream.stop = 0;
  devh->stream.seq = 0;
  devh->stream.fid = 0;
  devh->stream.pts = 0;
  devh->stream.last_scr = 0;
  devh->stream.outbuf = malloc(8 * 1024 * 1024); /** @todo take only what we need */
  devh->stream.holdbuf = malloc(8 * 1024 * 1024);

  if (isochronous) {
    /* For isochronous streaming, we choose an appropriate altsetting for the endpoint
     * and set up several transfers */
    const struct libusb_interface_descriptor *altsetting;
    const struct libusb_endpoint_descriptor *endpoint;
    /* The greatest number of bytes that the device might provide, per packet, in this
     * configuration */
    size_t config_bytes_per_packet;
    /* Number of packets per transfer */
    size_t packets_per_transfer;
    /* Total amount of data per transfer */
    size_t total_transfer_size;
    /* Size of packet transferable from the chosen endpoint */
    size_t endpoint_bytes_per_packet;
    /* Index of the altsetting */
    int alt_idx, ep_idx;
    
    struct libusb_transfer *transfer;
    int transfer_id;

    /* If the interface doesn't support isochronous mode, give up */
    if (interface->num_altsetting == 0)
      return UVC_ERROR_INVALID_DEVICE;

    config_bytes_per_packet = devh->cur_ctrl.dwMaxPayloadTransferSize;

    /* Go through the altsettings and find one whose packets are at least
     * as big as our format's maximum per-packet usage. Assume that the
     * packet sizes are increasing. */
    for (alt_idx = 0; alt_idx < interface->num_altsetting; alt_idx++) {
      altsetting = interface->altsetting + alt_idx;
      endpoint_bytes_per_packet = 0;

      /* Find the endpoint with the number specified in the VS header */
      for (ep_idx = 0; ep_idx < altsetting->bNumEndpoints; ep_idx++) {
        endpoint = altsetting->endpoint + ep_idx;

        if (endpoint->bEndpointAddress == devh->info->stream_ifs->bEndpointAddress) {
          endpoint_bytes_per_packet = endpoint->wMaxPacketSize;
          // wMaxPacketSize: [unused:2 (multiplier-1):3 size:11]
          endpoint_bytes_per_packet = (endpoint_bytes_per_packet & 0x07ff) *
                                      (((endpoint_bytes_per_packet >> 11) & 3) + 1);
          break;
        }
      }

      if (endpoint_bytes_per_packet >= config_bytes_per_packet) {
        /* Transfers will be at most one frame long: Divide the maximum frame size
         * by the size of the endpoint and round up */
        packets_per_transfer = (ctrl->dwMaxVideoFrameSize +
                                endpoint_bytes_per_packet - 1) / endpoint_bytes_per_packet;

        /* But keep a reasonable limit: Otherwise we start dropping data */
        if (packets_per_transfer > 32)
          packets_per_transfer = 32;
        
        total_transfer_size = packets_per_transfer * endpoint_bytes_per_packet;
        break;
      }
    }

    /* If we searched through all the altsettings and found nothing usable */
    if (alt_idx == interface->num_altsetting)
      return UVC_ERROR_INVALID_MODE;

    /* Select the altsetting */
    ret = libusb_set_interface_alt_setting(devh->usb_devh,
                                           altsetting->bInterfaceNumber,
                                           altsetting->bAlternateSetting);
    if (ret != UVC_SUCCESS)
      return ret;

    /* Set up the transfers */
    for (transfer_id = 0;
         transfer_id < sizeof(devh->stream.transfers) / sizeof(devh->stream.transfers[0]);
         ++transfer_id) {
      transfer = libusb_alloc_transfer(packets_per_transfer);
      devh->stream.transfers[transfer_id] = transfer;      
      devh->stream.transfer_bufs[transfer_id] = malloc(total_transfer_size);

      libusb_fill_iso_transfer(
        transfer, devh->usb_devh, devh->info->stream_ifs->bEndpointAddress,
        devh->stream.transfer_bufs[transfer_id],
        total_transfer_size, packets_per_transfer, _uvc_iso_callback, (void*) devh, 5000);

      libusb_set_iso_packet_lengths(transfer, endpoint_bytes_per_packet);
    }
  } else {
    /** @todo prepare for bulk transfer */
  }
   
  pthread_mutex_init(&devh->stream.cb_mutex, NULL);
  pthread_cond_init(&devh->stream.cb_cond, NULL);

  devh->stream.user_cb = cb;
  devh->stream.user_ptr = user_ptr;

  /* If the user wants it, set up a thread that calls the user's function
   * with the contents of each frame.
   */
  if (cb) {
    pthread_create(&devh->stream.cb_thread, NULL, _uvc_user_caller, (void*) devh);
  }

  if (isochronous) {
    int transfer_id;

    for (transfer_id = 0;
         transfer_id < sizeof(devh->stream.transfers) / sizeof(devh->stream.transfers[0]);
         transfer_id++) {
      ret = libusb_submit_transfer(devh->stream.transfers[transfer_id]);
      if (ret != UVC_SUCCESS)
        break;
    }
  } else {
    /** @todo submit bulk transfer */
  }

  if (ret) {
    /** @todo clean up transfers and memory */
  } else {
    devh->streaming = 1;
  }

  return ret;
}

/** Begin streaming video from the camera into the callback function.
 * @ingroup streaming
 *
 * @param devh UVC device
 * @param ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 */
uvc_error_t uvc_start_iso_streaming(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uvc_frame_callback_t *cb,
    void *user_ptr
) {
  return uvc_start_streaming(devh, ctrl, cb, user_ptr, 1);
}

/** @internal
 * @brief User callback runner thread
 * @note There should be at most one of these per currently streaming device
 * @param arg Device handle
 */
void *_uvc_user_caller(void *arg) {
  uvc_device_handle_t *devh = (uvc_device_handle_t *) arg;

  uint32_t last_seq = 0;

  do {
    pthread_mutex_lock(&devh->stream.cb_mutex);

    while (!devh->stream.stop && last_seq == devh->stream.hold_seq) {
      pthread_cond_wait(&devh->stream.cb_cond, &devh->stream.cb_mutex);
    }

    if (devh->stream.stop) {
      pthread_mutex_unlock(&devh->stream.cb_mutex);
      break;
    }
    
    last_seq = devh->stream.hold_seq;
    _uvc_populate_frame(devh);
    
    pthread_mutex_unlock(&devh->stream.cb_mutex);
    
    devh->stream.user_cb(&devh->stream.frame, devh->stream.user_ptr);
  } while(1);
}

/** @internal
 * @brief Populate the fields of a frame to be handed to user code
 * must be called with stream cb lock held!
 */
void _uvc_populate_frame(uvc_device_handle_t *devh) {
  size_t alloc_size = devh->cur_ctrl.dwMaxVideoFrameSize;
  uvc_frame_t *frame = &devh->stream.frame;
  uvc_frame_desc_t *frame_desc;

  /** @todo this stuff that hits the main config cache should really happen
   * in start() so that only one thread hits these data. all of this stuff
   * is going to be reopen_on_change anyway
   */

  frame_desc = uvc_find_frame_desc(devh, devh->cur_ctrl.bFormatIndex,
				   devh->cur_ctrl.bFrameIndex);

  frame->color_format = devh->stream.color_format;
  
  frame->width = frame_desc->wWidth;
  frame->height = frame_desc->wHeight;
  
  switch (frame->color_format) {
  case UVC_COLOR_FORMAT_YUYV:
    frame->step = frame->width * 2;
    break;
  case UVC_COLOR_FORMAT_MJPEG:
    frame->step = 0;
    break;
  default:
    frame->step = 0;
    break;
  }
  
  /* copy the image data from the hold buffer to the frame (unnecessary extra buf?) */
  if (frame->data_bytes < devh->stream.hold_bytes) {
    frame->data = realloc(frame->data, devh->stream.hold_bytes);
    frame->data_bytes = devh->stream.hold_bytes;
  }
  memcpy(frame->data, devh->stream.holdbuf, frame->data_bytes);
  
  /** @todo set the frame time */
}

/** Poll for a frame
 * @ingroup streaming
 *
 * @param devh UVC device
 * @param[out] frame Location to store pointer to captured frame (NULL on error)
 * @param timeout_us >0: Wait at most N microseconds; 0: Wait indefinitely; -1: return immediately
 */
uvc_error_t uvc_get_frame(uvc_device_handle_t *devh,
			  uvc_frame_t **frame,
			  int32_t timeout_us) {
  time_t add_secs;
  time_t add_nsecs;
  struct timespec ts;
  struct timeval tv;

  if (!devh->streaming)
    return UVC_ERROR_INVALID_PARAM;

  if (devh->stream.user_cb)
    return UVC_ERROR_CALLBACK_EXISTS;

  pthread_mutex_lock(&devh->stream.cb_mutex);

  if (devh->stream.last_polled_seq < devh->stream.hold_seq) {
    _uvc_populate_frame(devh);
    *frame = &devh->stream.frame;
    devh->stream.last_polled_seq = devh->stream.hold_seq;
  } else if (timeout_us != -1) {
    if (timeout_us == 0) {
      pthread_cond_wait(&devh->stream.cb_cond, &devh->stream.cb_mutex);
    } else {
      add_secs = timeout_us / 1000000;
      add_nsecs = (timeout_us % 1000000) * 1000;
      ts.tv_sec = 0;
      ts.tv_nsec = 0;

#if _POSIX_TIMERS > 0
      clock_gettime(CLOCK_REALTIME, &ts);
#else
      gettimeofday(&tv, NULL);
      ts.tv_sec = tv.tv_sec;
      ts.tv_nsec = tv.tv_usec * 1000;
#endif

      ts.tv_sec += add_secs;
      ts.tv_nsec += add_nsecs;

      pthread_cond_timedwait(&devh->stream.cb_cond, &devh->stream.cb_mutex, &ts);
    }
    
    if (devh->stream.last_polled_seq < devh->stream.hold_seq) {
      _uvc_populate_frame(devh);
      *frame = &devh->stream.frame;
      devh->stream.last_polled_seq = devh->stream.hold_seq;
    } else {
      *frame = NULL;
    }
  } else {
    *frame = NULL;
  }

  pthread_mutex_unlock(&devh->stream.cb_mutex);

  return UVC_SUCCESS;
}

/** @brief Stop streaming video
 * @ingroup streaming
 *
 * Closes stream, ends threads and cancels pollers
 *
 * @param devh UVC device
 */
void uvc_stop_streaming(uvc_device_handle_t *devh) {
  int transfer_idx;

  if (!devh->streaming)
    return;
  
  devh->stream.stop = 1;

  for (transfer_idx = 0; transfer_idx < 5; ++transfer_idx) {
    if (devh->stream.transfers[transfer_idx]) {
      if (libusb_cancel_transfer(devh->stream.transfers[transfer_idx])
	  == LIBUSB_ERROR_NOT_FOUND) {
	free(devh->stream.transfers[transfer_idx]->buffer);
	libusb_free_transfer(devh->stream.transfers[transfer_idx]);
      }

      /* callback will free any remaining transfers as they come in */
    }
  }

  /** @todo stop the actual stream, camera side? */

  if (devh->stream.user_cb) {
    /* wait for the thread to stop (triggered by
     * LIBUSB_TRANSFER_CANCELLED transfer) */
    pthread_join(devh->stream.cb_thread, NULL);
  }

  free(devh->stream.outbuf);
  free(devh->stream.holdbuf);

  pthread_cond_destroy(&devh->stream.cb_cond);
  pthread_mutex_destroy(&devh->stream.cb_mutex);
  
  devh->streaming = 0;
}

