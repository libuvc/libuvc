#include <stdio.h>
#include <opencv/highgui.h>

#include "libuvc/libuvc.h"

void cb(uvc_frame_t *frame, void *ptr) {
  uvc_frame_t *bgr;
  uvc_error_t ret;
  IplImage* cvImg;

  printf("callback! length = %u, ptr = %d\n", frame->data_bytes, (int) ptr);

  bgr = uvc_allocate_frame(frame->width * frame->height * 3);
  if (!bgr) {
    printf("unable to allocate bgr frame!");
    return;
  }

  ret = uvc_any2bgr(frame, bgr);
  if (ret) {
    uvc_perror(ret, "uvc_any2bgr");
    uvc_free_frame(bgr);
    return;
  }

  cvImg = cvCreateImageHeader(
      cvSize(bgr->width, bgr->height),
      IPL_DEPTH_8U,
      3);

  cvSetData(cvImg, bgr->data, bgr->width * 3); 

  cvNamedWindow("Test", CV_WINDOW_AUTOSIZE);
  cvShowImage("Test", cvImg);
  cvWaitKey(10);

  cvReleaseImageHeader(&cvImg);

  uvc_free_frame(bgr);
}

int main(int argc, char **argv) {
  uvc_context_t *ctx;
  uvc_error_t res;
  uvc_device_t *dev;
  uvc_device_handle_t *devh;
  uvc_stream_ctrl_t ctrl;

  res = uvc_init(&ctx, NULL);

  if (res < 0) {
    uvc_perror(res, "uvc_init");
    return res;
  }

  puts("UVC initialized");

  res = uvc_find_device(
      ctx, &dev,
      0, 0, NULL);

  if (res < 0) {
    uvc_perror(res, "uvc_find_device");
  } else {
    puts("Device found");

    res = uvc_open(dev, &devh);

    if (res < 0) {
      uvc_perror(res, "uvc_open");
    } else {
      puts("Device opened");

      uvc_print_diag(devh, stderr);

      res = uvc_get_stream_ctrl_format_size(
          devh, &ctrl, UVC_COLOR_FORMAT_YUYV, 640, 480, 30
      );

      uvc_print_stream_ctrl(&ctrl, stderr);

      if (res < 0) {
        uvc_perror(res, "get_mode");
      } else {
        res = uvc_start_iso_streaming(devh, &ctrl, cb, 12345);

        if (res < 0) {
          uvc_perror(res, "start_streaming");
        } else {
          puts("Streaming for 10 seconds...");
          uvc_error_t resAEMODE = uvc_set_ae_mode(devh, 1);
          uvc_perror(resAEMODE, "set_ae_mode");
          int i;
          for (i = 1; i <= 10; i++) {
            /* uvc_error_t resPT = uvc_set_pantilt_abs(devh, i * 20 * 3600, 0); */
            /* uvc_perror(resPT, "set_pt_abs"); */
            uvc_error_t resEXP = uvc_set_exposure_abs(devh, 20 + i * 5);
            uvc_perror(resEXP, "set_exp_abs");
            
            sleep(1);
          }
          sleep(10);
          uvc_stop_streaming(devh);
	  puts("Done streaming.");
        }
      }

      uvc_close(devh);
      puts("Device closed");
    }

    uvc_unref_device(dev);
  }

  uvc_exit(ctx);
  puts("UVC exited");

  return 0;
}

