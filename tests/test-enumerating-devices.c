#include "libuvc/libuvc.h"
#include <stdio.h>
#include <stdlib.h> // putenv

#define DL_FOREACH(head, el) \
  for (el = head; el; el = el->next)

int interval_to_fps(int interval)
{
  return (int)(10000000. / interval);
}

int get_product_name(uvc_device_t *dev)
{
  uvc_error_t res;
  uvc_device_descriptor_t *desc;
  if ((res = uvc_get_device_descriptor(dev, &desc)) == UVC_SUCCESS)
  {

    fprintf(stderr, "\n>>>> %s\n", desc->product);
    uvc_free_device_descriptor(desc);
    return 0;
  }
  uvc_perror(res, "get_product_name");
  return -10001;
}

int load_device_config(uvc_device_t *dev)
{
  int should_detach_kernel_driver = 0;
  uvc_error_t res;
  uvc_device_handle_t *devh;

  res = uvc_open(dev, &devh, 1);
  if (res != UVC_SUCCESS)
  {
    uvc_perror(res, "load_device_config");
    return -10002;
  }

  uvc_stream_ctrl_t ctrl;
  const uvc_format_desc_t *format;
  const uvc_frame_desc_t *frame;

  DL_FOREACH(uvc_get_format_descs(devh), format)
  {
    DL_FOREACH(format->frame_descs, frame)
    {
      fprintf(
          stderr,
          ">>> Frame format w=%hu h=%hu fps=%hu\n",
          frame->wWidth,
          frame->wHeight,
          interval_to_fps(frame->intervals[0]));

      res = uvc_get_stream_ctrl_format_size(
          devh,
          &ctrl,
          UVC_FRAME_FORMAT_ANY,
          frame->wWidth,
          frame->wHeight,
          interval_to_fps(frame->intervals[0]),
          should_detach_kernel_driver);
      break;
    }
    break;
  }

  uvc_stream_handle_t *strmh;

  uvc_print_stream_ctrl(&ctrl, stderr);

  res = uvc_stream_open_ctrl(devh, &strmh, &ctrl, should_detach_kernel_driver);
  if (res != UVC_SUCCESS)
    return res;

  res = uvc_stream_start(strmh, NULL, NULL, 2, 0);
  if (res != UVC_SUCCESS)
  {
    uvc_stream_close(strmh);
    return res;
  }

  uvc_frame_t *uvc_frame = NULL;
  int num_frames_total = 0;
  int num_frames_broken = 0;

  for (; num_frames_total < 1000; num_frames_total++)
  {
    res = uvc_stream_get_frame(strmh, &uvc_frame, 10000);
    if (res != UVC_SUCCESS)
    {
      uvc_perror(res, "uvc_stream_get_frame");
    }
    else if (uvc_frame->width * uvc_frame->height > uvc_frame->data_bytes)
    {
      num_frames_broken++;
    }
  }
  fprintf(stderr, "++++++++++++++++++++++\n");
  fprintf(stderr, "%i / %i (%f %%) broken\n", num_frames_broken, num_frames_total, 100 * num_frames_broken / num_frames_total);

  uvc_stream_stop(strmh);
  uvc_stream_close(strmh);
  uvc_close(devh);
  return 0;
}

int enumerate_devices()
{
  uvc_context_t *ctx;
  uvc_error_t res;

  /* Initialize a UVC service context. Libuvc will set up its own libusb
   * context. Replace NULL with a libusb_context pointer to run libuvc
   * from an existing libusb context. */
  res = uvc_init(&ctx, NULL);

  if (res < 0)
  {
    uvc_perror(res, "uvc_init");
    return res;
  }

  puts("UVC initialized");

  uvc_device_t **dev_list;
  uvc_device_t *dev;

  res = uvc_get_device_list(ctx, &dev_list);
  if (res < 0)
  {
    uvc_perror(res, "uvc_get_device_list");
    return res;
  }

  int idx = -1;
  int subtest_result;
  while ((dev = dev_list[++idx]) != NULL)
  {
    if ((subtest_result = get_product_name(dev)) < 0)
    {
      return subtest_result;
    }
    if ((subtest_result = load_device_config(dev)) < 0)
    {
      return subtest_result;
    }
  }

  uvc_free_device_list(dev_list, 1);

  /* Close the UVC context. This closes and cleans up any existing device handles,
   * and it closes the libusb context if one was not provided. */
  uvc_exit(ctx);
  puts("UVC exited");
  return 0;
}

int main(int argc, char **argv)
{
  // putenv("LIBUSB_DEBUG=4");
  return enumerate_devices();
}
