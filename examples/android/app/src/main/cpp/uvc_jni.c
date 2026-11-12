/*
 * JNI glue between UvcCamera.kt and libuvc.
 *
 * The Kotlin side owns the USB device: it gets permission from UsbManager,
 * opens a UsbDeviceConnection and passes its file descriptor here. libuvc
 * wraps that descriptor (uvc_wrap) instead of opening the device itself,
 * which an app cannot do on Android.
 */
#include <ctype.h>
#include <jni.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <android/imagedecoder.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <libusb.h>
#include <libuvc/libuvc.h>

#define TAG "libuvc-example"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

#define MAX_MODES 128

struct mode {
  enum uvc_frame_format format;
  int width, height, fps;
  char label[48];
};

struct camera {
  uvc_context_t *ctx;
  uvc_device_handle_t *devh;
  int streaming;

  struct mode modes[MAX_MODES];
  int num_modes, default_mode;

  /* Shared with frame_callback(), which runs on a libuvc thread. */
  pthread_mutex_t lock;
  ANativeWindow *window;
  int window_width, window_height;
  uvc_frame_t *rgb;
  unsigned errors;
  atomic_uint frames;
};

static struct camera *from_handle(jlong handle) {
  return (struct camera *) (intptr_t) handle;
}

static void throw_exception(JNIEnv *env, const char *class_name,
                            const char *message) {
  jclass clazz = (*env)->FindClass(env, class_name);

  if (clazz)
    (*env)->ThrowNew(env, clazz, message);
}

static void throw_uvc_error(JNIEnv *env, const char *what, uvc_error_t res) {
  char message[128];

  snprintf(message, sizeof(message), "%s: %s (%d)", what, uvc_strerror(res),
           res);
  throw_exception(env, "java/io/IOException", message);
}

/* Pick a frame interval that uvc_get_stream_ctrl_format_size() will accept
 * back: the default one, unless the camera does not list it. */
static uint32_t pick_interval(const uvc_frame_desc_t *frame) {
  uint32_t *interval;

  if (!frame->intervals)
    return frame->dwDefaultFrameInterval ? frame->dwDefaultFrameInterval
                                         : frame->dwMinFrameInterval;

  for (interval = frame->intervals; *interval; interval++) {
    if (*interval == frame->dwDefaultFrameInterval)
      return *interval;
  }
  return frame->intervals[0];
}

/* MJPEG is decoded by the platform's AImageDecoder, the rest converted by
 * libuvc's uvc_any2rgb(). Nearly every webcam offers one of these; H.264, or
 * the greyscale formats of thermal cameras, are not handled. */
static int is_supported(enum uvc_frame_format format) {
  switch (format) {
  case UVC_FRAME_FORMAT_MJPEG:
  case UVC_FRAME_FORMAT_YUYV:
  case UVC_FRAME_FORMAT_UYVY:
  case UVC_FRAME_FORMAT_RGB:
    return 1;
  default:
    return 0;
  }
}

/* List every frame size of every format the camera offers and this example
 * can show. The default is the first such format's default frame size, the
 * mode the camera itself suggests. */
static void find_modes(struct camera *cam) {
  const uvc_format_desc_t *format;

  cam->default_mode = -1;

  for (format = uvc_get_format_descs(cam->devh); format;
       format = format->next) {
    /* uvc_frame_format_for_guid() takes a non-const pointer. */
    enum uvc_frame_format frame_format =
        uvc_frame_format_for_guid((uint8_t *) format->guidFormat);
    const uvc_frame_desc_t *frame;
    char fourcc[5];

    for (int i = 0; i < 4; i++) {
      uint8_t c = format->fourccFormat[i];

      fourcc[i] = isprint(c) ? c : '?';
    }
    fourcc[4] = '\0';

    if (!is_supported(frame_format)) {
      LOGI("Skipping format %s: not supported by this example", fourcc);
      continue;
    }

    for (frame = format->frame_descs; frame; frame = frame->next) {
      uint32_t interval = pick_interval(frame);
      struct mode *mode;

      if (!interval)
        continue;
      if (cam->num_modes == MAX_MODES)
        goto out;

      mode = &cam->modes[cam->num_modes];
      mode->format = frame_format;
      mode->width = frame->wWidth;
      mode->height = frame->wHeight;
      /* Intervals are in 100 ns units. Rounded down, as
       * uvc_get_stream_ctrl_format_size() compares it. */
      mode->fps = 10000000 / interval;
      snprintf(mode->label, sizeof(mode->label), "%s %dx%d, %.1f fps", fourcc,
               mode->width, mode->height, 10000000.0 / interval);
      LOGI("Mode %d: %s", cam->num_modes, mode->label);

      if (cam->default_mode < 0 &&
          frame->bFrameIndex == format->bDefaultFrameIndex)
        cam->default_mode = cam->num_modes;
      cam->num_modes++;
    }
  }

out:
  if (cam->default_mode < 0)
    cam->default_mode = 0;
}

/* Decode into the window buffer, which has the frame's size. AImageDecoder
 * copes with the Huffman tables that UVC cameras usually leave out of their
 * MJPEG frames. */
static int decode_mjpeg(uvc_frame_t *frame, const ANativeWindow_Buffer *buffer) {
  const size_t stride = (size_t) buffer->stride * 4;
  const AImageDecoderHeaderInfo *info;
  AImageDecoder *decoder;
  int ret;

  ret = AImageDecoder_createFromBuffer(frame->data, frame->data_bytes,
                                       &decoder);
  if (ret != ANDROID_IMAGE_DECODER_SUCCESS)
    return ret;

  /* A frame of another size than negotiated would overrun the buffer. */
  info = AImageDecoder_getHeaderInfo(decoder);
  if (AImageDecoderHeaderInfo_getWidth(info) != buffer->width ||
      AImageDecoderHeaderInfo_getHeight(info) != buffer->height)
    ret = ANDROID_IMAGE_DECODER_BAD_PARAMETER;
  else
    ret = AImageDecoder_decodeImage(decoder, buffer->bits, stride,
                                    stride * buffer->height);

  AImageDecoder_delete(decoder);
  return ret;
}

/* Convert with libuvc, then copy the RGB pixels to the window buffer, which
 * has an X byte after each of them. */
static int convert_rgb(struct camera *cam, uvc_frame_t *frame,
                       const ANativeWindow_Buffer *buffer) {
  uvc_error_t res = uvc_any2rgb(frame, cam->rgb);

  if (res < 0)
    return res;
  /* RGB frames are only copied, not checked against their size. */
  if (cam->rgb->data_bytes < (size_t) buffer->width * buffer->height * 3)
    return UVC_ERROR_OTHER;

  for (int y = 0; y < buffer->height; y++) {
    const uint8_t *src =
        (uint8_t *) cam->rgb->data + (size_t) y * buffer->width * 3;
    uint8_t *dst = (uint8_t *) buffer->bits + (size_t) y * buffer->stride * 4;

    for (int x = 0; x < buffer->width; x++, src += 3, dst += 4) {
      dst[0] = src[0];
      dst[1] = src[1];
      dst[2] = src[2];
    }
  }
  return 0;
}

/* Called by libuvc, on its own thread, for every complete frame. */
static void frame_callback(uvc_frame_t *frame, void *user_ptr) {
  struct camera *cam = user_ptr;
  ANativeWindow_Buffer buffer;
  int ret;

  atomic_fetch_add(&cam->frames, 1);

  pthread_mutex_lock(&cam->lock);
  if (!cam->window)
    goto out;

  /* Size the window buffers like the frames: the SurfaceView scales them to
   * the screen, for free. */
  if (cam->window_width != (int) frame->width ||
      cam->window_height != (int) frame->height) {
    if (ANativeWindow_setBuffersGeometry(cam->window, frame->width,
                                         frame->height,
                                         WINDOW_FORMAT_RGBX_8888))
      goto out;
    cam->window_width = frame->width;
    cam->window_height = frame->height;
  }

  if (ANativeWindow_lock(cam->window, &buffer, NULL))
    goto out;
  if (buffer.width != (int32_t) frame->width ||
      buffer.height != (int32_t) frame->height)
    ret = UVC_ERROR_OTHER;
  else if (frame->frame_format == UVC_FRAME_FORMAT_MJPEG)
    ret = decode_mjpeg(frame, &buffer);
  else
    ret = convert_rgb(cam, frame, &buffer);
  ANativeWindow_unlockAndPost(cam->window);

  if (ret && cam->errors++ % 100 == 0)
    LOGW("Cannot show frame %u (%zu bytes): %d (%u so far)", frame->sequence,
         frame->data_bytes, ret, cam->errors);

out:
  pthread_mutex_unlock(&cam->lock);
}

static jlong camera_open(JNIEnv *env, jclass clazz, jint fd) {
  struct camera *cam = calloc(1, sizeof(*cam));
  uvc_error_t res;

  /* Empty: uvc_any2rgb() sizes it for the frames. */
  if (cam)
    cam->rgb = uvc_allocate_frame(0);
  if (!cam || !cam->rgb) {
    free(cam);
    throw_exception(env, "java/lang/OutOfMemoryError", "struct camera");
    return 0;
  }
  pthread_mutex_init(&cam->lock, NULL);
  atomic_init(&cam->frames, 0);

  /* With a NULL libusb context, libuvc creates its own, and runs the thread
   * that handles its events from the first uvc_wrap() to the last
   * uvc_close(). That context does not scan for devices: JNI_OnLoad() set
   * LIBUSB_OPTION_NO_DEVICE_DISCOVERY as a default for every new context.
   * Passing a context of our own instead, created with that option, would
   * leave handling its events to us. */
  res = uvc_init(&cam->ctx, NULL);
  if (res < 0) {
    throw_uvc_error(env, "uvc_init", res);
    goto fail;
  }

  /* fd stays owned by the UsbDeviceConnection: uvc_close() does not close
   * it, and the connection must stay open until then. */
  res = uvc_wrap(fd, cam->ctx, &cam->devh);
  if (res < 0) {
    throw_uvc_error(env, "uvc_wrap", res);
    goto fail_exit;
  }

  find_modes(cam);
  return (jlong) (intptr_t) cam;

fail_exit:
  uvc_exit(cam->ctx);
fail:
  pthread_mutex_destroy(&cam->lock);
  uvc_free_frame(cam->rgb);
  free(cam);
  return 0;
}

static jobjectArray camera_modes(JNIEnv *env, jclass clazz, jlong handle) {
  struct camera *cam = from_handle(handle);
  jclass string_class = (*env)->FindClass(env, "java/lang/String");
  jobjectArray labels;

  if (!string_class)
    return NULL;
  labels = (*env)->NewObjectArray(env, cam->num_modes, string_class, NULL);
  for (int i = 0; labels && i < cam->num_modes; i++) {
    jstring label = (*env)->NewStringUTF(env, cam->modes[i].label);

    if (!label)
      return NULL;
    (*env)->SetObjectArrayElement(env, labels, i, label);
    (*env)->DeleteLocalRef(env, label);
  }
  return labels;
}

static jint camera_default_mode(JNIEnv *env, jclass clazz, jlong handle) {
  return from_handle(handle)->default_mode;
}

static void camera_stop(JNIEnv *env, jclass clazz, jlong handle) {
  struct camera *cam = from_handle(handle);

  if (cam->streaming) {
    /* Waits for frame_callback() to return, so it must never be called from
     * there. */
    uvc_stop_streaming(cam->devh);
    cam->streaming = 0;
  }
}

static jintArray camera_start(JNIEnv *env, jclass clazz, jlong handle,
                              jint index) {
  struct camera *cam = from_handle(handle);
  const struct mode *mode;
  uvc_stream_ctrl_t ctrl;
  uvc_error_t res;
  jintArray size;

  if (index < 0 || index >= cam->num_modes) {
    throw_exception(env, "java/lang/IllegalArgumentException", "mode");
    return NULL;
  }
  mode = &cam->modes[index];

  camera_stop(env, clazz, handle);

  /* Negotiate the mode with the camera (the UVC probe and commit). */
  res = uvc_get_stream_ctrl_format_size(cam->devh, &ctrl, mode->format,
                                        mode->width, mode->height, mode->fps);
  if (res < 0) {
    throw_uvc_error(env, "uvc_get_stream_ctrl_format_size", res);
    return NULL;
  }

  res = uvc_start_streaming(cam->devh, &ctrl, frame_callback, cam, 0);
  if (res < 0) {
    throw_uvc_error(env, "uvc_start_streaming", res);
    return NULL;
  }
  cam->streaming = 1;
  LOGI("Streaming %s", mode->label);

  size = (*env)->NewIntArray(env, 2);
  if (size)
    (*env)->SetIntArrayRegion(env, size, 0, 2,
                              (jint[]) { mode->width, mode->height });
  return size;
}

static void camera_set_surface(JNIEnv *env, jclass clazz, jlong handle,
                               jobject surface) {
  struct camera *cam = from_handle(handle);
  ANativeWindow *window = surface ? ANativeWindow_fromSurface(env, surface)
                                  : NULL;
  ANativeWindow *old;

  pthread_mutex_lock(&cam->lock);
  old = cam->window;
  cam->window = window;
  cam->window_width = cam->window_height = 0;
  pthread_mutex_unlock(&cam->lock);

  if (old)
    ANativeWindow_release(old);
}

static jint camera_frame_count(JNIEnv *env, jclass clazz, jlong handle) {
  return atomic_load(&from_handle(handle)->frames);
}

static void camera_close(JNIEnv *env, jclass clazz, jlong handle) {
  struct camera *cam = from_handle(handle);

  camera_stop(env, clazz, handle);
  /* Releases the interfaces, handing them back to the kernel driver if
   * libuvc had to detach it, and stops the event thread. */
  uvc_close(cam->devh);
  uvc_exit(cam->ctx);

  if (cam->window)
    ANativeWindow_release(cam->window);
  pthread_mutex_destroy(&cam->lock);
  uvc_free_frame(cam->rgb);
  free(cam);
}

static const JNINativeMethod methods[] = {
  { "nativeOpen", "(I)J", camera_open },
  { "nativeModes", "(J)[Ljava/lang/String;", camera_modes },
  { "nativeDefaultMode", "(J)I", camera_default_mode },
  { "nativeStart", "(JI)[I", camera_start },
  { "nativeStop", "(J)V", camera_stop },
  { "nativeSetSurface", "(JLandroid/view/Surface;)V", camera_set_surface },
  { "nativeFrameCount", "(J)I", camera_frame_count },
  { "nativeClose", "(J)V", camera_close },
};

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  JNIEnv *env;
  jclass clazz;

  if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK)
    return JNI_ERR;

  clazz = (*env)->FindClass(env, "io/github/libuvc/example/UvcCamera");
  if (!clazz ||
      (*env)->RegisterNatives(env, clazz, methods,
                              sizeof(methods) / sizeof(methods[0])))
    return JNI_ERR;

  /* An app cannot list /dev/bus/usb on Android, only use the file
   * descriptors UsbManager hands out: stop libusb from scanning for devices.
   * Set as a default, before any libusb context exists, as it then applies to
   * the one uvc_init() creates. */
  libusb_set_option(NULL, LIBUSB_OPTION_NO_DEVICE_DISCOVERY);
  /* libusb logs to logcat, with the "libusb" tag. */
  libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

  return JNI_VERSION_1_6;
}
