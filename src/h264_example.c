#include "libuvc/libuvc.h"
#include <stdio.h>

static const char *H264_FILE = "iOSDevLog.h264";
static const char *FORMAT_STRS[] = {
  /** Any supported format */
  "UVC_FRAME_FORMAT_ANY",
  "UVC_FRAME_FORMAT_UNCOMPRESSED",
  "UVC_FRAME_FORMAT_COMPRESSED",
  /** YUYV/YUV2/YUV422: YUV encoding with one luminance value per pixel and
   * one UV (chrominance) pair for every two pixels.
   */
  "UVC_FRAME_FORMAT_YUYV",
  "UVC_FRAME_FORMAT_UYVY",
  /** 24-bit RGB */
  "UVC_FRAME_FORMAT_RGB",
  "UVC_FRAME_FORMAT_BGR",
  /** Motion-JPEG (or JPEG) encoded images */
  "UVC_FRAME_FORMAT_MJPEG",
  "UVC_FRAME_FORMAT_H264",
  /** Greyscale images */
  "UVC_FRAME_FORMAT_GRAY8",
  "UVC_FRAME_FORMAT_GRAY16",
  /* Raw colour mosaic images */
  "UVC_FRAME_FORMAT_BY8",
  "UVC_FRAME_FORMAT_BA81",
  "UVC_FRAME_FORMAT_SGRBG8",
  "UVC_FRAME_FORMAT_SGBRG8",
  "UVC_FRAME_FORMAT_SRGGB8",
  "UVC_FRAME_FORMAT_SBGGR8",
  /** YUV420: NV12 */
  "UVC_FRAME_FORMAT_NV12",
  /** Number of formats understood */
  "UVC_FRAME_FORMAT_COUNT",
};

/* This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
void cb(uvc_frame_t *frame, void *ptr) {
  uvc_frame_t *bgr;
  uvc_error_t ret;
  int *count_ptr = (int *)ptr;

  /* We'll convert the image from YUV/JPEG to BGR, so allocate space */
  bgr = uvc_allocate_frame(frame->width * frame->height * 3);
  if (!bgr) {
    printf("unable to allocate bgr frame!");
    return;
  }

  /* Do the BGR conversion */
  /*
   * ret = uvc_any2bgr(frame, bgr);
   * if (ret) {
   *   uvc_perror(ret, "uvc_any2bgr");
   *   uvc_free_frame(bgr);
   *   return;
   * }
   */

  printf("frame_count = %d, width = %d, height = %d, frame_format = %s\n",
          (*count_ptr)++, frame->width, frame->height, FORMAT_STRS[frame->frame_format]);

  /* user `ffplay iOSDevLog.h264` to play */
  FILE *fp;
  fp = fopen(H264_FILE, "a");
  fwrite(frame->data, 1, frame->data_bytes, fp);
  fclose(fp);

  /* Call a user function:
   *
   * my_type *my_obj = (*my_type) ptr;
   * my_user_function(ptr, bgr);
   * my_other_function(ptr, bgr->data, bgr->width, bgr->height);
   */

  /* Call a C++ method:
   *
   * my_type *my_obj = (*my_type) ptr;
   * my_obj->my_func(bgr);
   */

  /* Use opencv.highgui to display the image:
   * 
   * cvImg = cvCreateImageHeader(
   *     cvSize(bgr->width, bgr->height),
   *     IPL_DEPTH_8U,
   *     3);
   *
   * cvSetData(cvImg, bgr->data, bgr->width * 3); 
   *
   * cvNamedWindow("Test", CV_WINDOW_AUTOSIZE);
   * cvShowImage("Test", cvImg);
   * cvWaitKey(10);
   *
   * cvReleaseImageHeader(&cvImg);
   */

  uvc_free_frame(bgr);
}

int main(int argc, char **argv) {
  uvc_context_t *ctx;
  uvc_device_t *dev;
  uvc_device_handle_t *devh;
  uvc_stream_ctrl_t ctrl;
  uvc_error_t res;
  int frame_count = 0;

  /* Initialize a UVC service context. Libuvc will set up its own libusb
   * context. Replace NULL with a libusb_context pointer to run libuvc
   * from an existing libusb context. */
  res = uvc_init(&ctx, NULL);

  if (res < 0) {
    uvc_perror(res, "uvc_init");
    return res;
  }

  puts("UVC initialized");

  /* Locates the first attached UVC device, stores in dev */
  res = uvc_find_device(
      ctx, &dev,
      0, 0, NULL); /* filter devices: vendor_id, product_id, "serial_num" */

  if (res < 0) {
    uvc_perror(res, "uvc_find_device"); /* no devices found */
  } else {
    puts("Device found");

    /* Try to open the device: requires exclusive access */
    res = uvc_open(dev, &devh);

    if (res < 0) {
      uvc_perror(res, "uvc_open"); /* unable to open device */
    } else {
      puts("Device opened");

      /* Print out a message containing all the information that libuvc
       * knows about the device */
      uvc_print_diag(devh, stderr);

      /* Try to negotiate a 848x480 30 fps YUYV stream profile */
      res = uvc_get_stream_ctrl_format_size(
          devh, &ctrl, /* result stored in ctrl */
          UVC_FRAME_FORMAT_H264, /* H264. try _COMPRESSED */
          848, 480, 30 /* width, height, fps */
      );

      /* Print out the result */
      uvc_print_stream_ctrl(&ctrl, stderr);

      if (res < 0) {
        uvc_perror(res, "get_mode"); /* device doesn't provide a matching stream */
      } else {
        /* Start the video stream. The library will call user function cb:
         *   cb(frame, (void*) 12345)
         */
        res = uvc_start_streaming(devh, &ctrl, cb, &frame_count, 0);

        if (res < 0) {
          uvc_perror(res, "start_streaming"); /* unable to start stream */
        } else {
          puts("Streaming...");

          uvc_set_ae_mode(devh, 1); /* e.g., turn on auto exposure */

          sleep(10); /* stream for 10 seconds */

          /* End the stream. Blocks until last callback is serviced */
          uvc_stop_streaming(devh);
          puts("Done streaming. ");
        }
      }

      /* Release our handle on the device */
      uvc_close(devh);
      puts("Device closed");
    }

    /* Release the device descriptor */
    uvc_unref_device(dev);
  }

  /* Close the UVC context. This closes and cleans up any existing device handles,
   * and it closes the libusb context if one was not provided. */
  uvc_exit(ctx);
  puts("UVC exited");

  printf("use `ffplay %s` to play", H264_FILE);

  return 0;
}

