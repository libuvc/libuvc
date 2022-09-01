#include "libuvc/libuvc.h"
#include <stdio.h>

int main(int argc, char **argv)
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
  uvc_device_descriptor_t *desc;

  res = uvc_get_device_list(ctx, &dev_list);
  if (res < 0)
  {
    uvc_perror(res, "uvc_get_device_list");
    return res;
  }

  /* Close the UVC context. This closes and cleans up any existing device handles,
   * and it closes the libusb context if one was not provided. */
  uvc_exit(ctx);
  puts("UVC exited");

  return 0;
}
