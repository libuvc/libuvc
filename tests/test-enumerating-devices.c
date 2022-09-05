#include "libuvc/libuvc.h"
#include <stdio.h>
#include <stdlib.h> // putenv

int get_product_name(uvc_device_t *dev)
{
  uvc_error_t res;
  uvc_device_descriptor_t *desc;
  if ((res = uvc_get_device_descriptor(dev, &desc)) == UVC_SUCCESS)
  {

    fprintf(stderr, ">>>> %s\n", desc->product);
    uvc_free_device_descriptor(desc);
    return 0;
  }
  uvc_perror(res, "get_product_name");
  return -10001;
}

int load_device_config(uvc_device_t *dev)
{
  uvc_error_t res;
  uvc_device_handle_t *devh;
  res = uvc_open(dev, &devh, 1);
  if (res != UVC_SUCCESS)
  {
    uvc_perror(res, "load_device_config");
    return -10002;
  }

  const uvc_format_desc_t *format_desc = uvc_get_format_descs(devh);

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
  putenv("LIBUSB_DEBUG=4");
  return enumerate_devices();
}
