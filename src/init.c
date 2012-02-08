/**
\mainpage
\htmlinclude manifest.html

\b libuvc is a library that supports enumeration, control and streaming
for USB Video Class (UVC) devices, such as consumer webcams.

*/

/**
 * @defgroup init Library initialization/deinitialization
 */
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

/** @internal
 * @brief Event handler thread
 * There's one of these per UVC context.
 * @todo We shouldn't run this if we don't own the USB context
 */
void *_uvc_handle_events(void *arg) {
  uvc_context_t *ctx = (uvc_context_t *) arg;

  while (!ctx->kill_handler_thread)
    libusb_handle_events(ctx->usb_ctx);
}

/** @brief Initializes the UVC context
 * @ingroup init
 *
 * @note If you provide your own USB context, you must handle
 * libusb event processing using a function such as libusb_handle_events.
 *
 * @param[out] pctx The location where the context reference should be stored.
 * @param[in]  usb_ctx Optional USB context to use
 * @return Error opening context or UVC_SUCCESS
 */
uvc_error_t uvc_init(uvc_context_t **pctx, struct libusb_context *usb_ctx) {
  uvc_error_t ret = UVC_SUCCESS;
  uvc_context_t *ctx = calloc(1, sizeof(*ctx));

  if (usb_ctx == NULL) {
    ret = libusb_init(&ctx->usb_ctx);
    ctx->own_usb_ctx = 1;
    if (ret != UVC_SUCCESS) {
      free(ctx);
      ctx = NULL;
    }
  } else {
    ctx->own_usb_ctx = 0;
    ctx->usb_ctx = usb_ctx;
  }

  if (ctx != NULL) {
    ctx->kill_handler_thread = 0;
    if (ctx->own_usb_ctx)
      pthread_create(&ctx->handler_thread, NULL, _uvc_handle_events, (void*) ctx);
    *pctx = ctx;
  }

  return ret;
}

/**
 * @brief Closes the UVC context, shutting down any active cameras.
 * @ingroup init
 *
 * @note This function invalides any existing references to the context's
 * cameras.
 *
 * If no USB context was provided to #uvc_init, the UVC-specific USB
 * context will be destroyed.
 *
 * @param ctx UVC context to shut down
 */
void uvc_exit(uvc_context_t *ctx) {
  uvc_device_handle_t *devh;

  DL_FOREACH(ctx->open_devices, devh) {
    uvc_close(devh);
  }

  ctx->kill_handler_thread = 1;

  if (ctx->own_usb_ctx) {
    pthread_kill(ctx->handler_thread, SIGINT);
    pthread_join(ctx->handler_thread, NULL);
    libusb_exit(ctx->usb_ctx);
  }

  free(ctx);
}

