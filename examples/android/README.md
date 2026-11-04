# libuvc on Android

**AI disclaimer: this example was put together by Claude, reviewed by human (`drinkcat`), there's often more than one way to integrate libraries into apps, so this is only one of multiple possible ways, but it looks simpler enough.**

A small Kotlin app that shows the picture of a USB (UVC) camera plugged into
an Android phone, using libuvc and libusb from this tree. It lists the formats
and frame sizes the camera offers, starts with the camera's default and lets
you pick another.

It shows MJPEG (Android 11 and later) and the uncompressed formats libuvc
knows: YUYV, UYVY, NV12, NV21, I420, P010, 8 and 16-bit grey, RGB, BGR, and
raw Bayer, in grey. Nearly every webcam offers one of those; a camera that only
streams H.264 is not handled.

## Building and running

Open this directory in Android Studio, or:

    ./gradlew assembleRelease
    adb install app/build/outputs/apk/release/app-release.apk

Gradle downloads libusb, and builds it and libuvc with the NDK. The release
build is signed with the debug key so that it installs as is; prefer it to the
debug build, whose native code is not optimised.

The phone needs USB host (OTG) support. The app asks for the camera permission,
then for permission to use the USB device. Android also offers to open the app
whenever a device with a video interface is plugged in, whether the app can
show its formats or not, thanks to the `USB_DEVICE_ATTACHED` filter in its
manifest.

The CI builds the APK on every pull request: it is attached to the
"android, example app" job as an artifact.

## Integrating libuvc in an app

The parts that differ from desktop Linux:

### Getting libusb and libuvc built

- libusb itself builds with autotools, or with ndk-build from its
  `android/jni/libusb.mk`. With CMake, use
  [libusb-cmake](https://github.com/libusb/libusb-cmake), the CMake build the
  libusb project maintains, which handles Android: fetch it, as
  [`CMakeLists.txt`](app/src/main/cpp/CMakeLists.txt) does. Keeping libusb a
  shared library of its own is the simple way to meet its license, the LGPL,
  which requires that users can replace it.
- Use libuvc with `add_subdirectory()`, after making libusb available as
  `LibUSB::LibUSB`, here an alias of libusb-cmake's target: libuvc then uses
  it rather than look for libusb with pkg-config, which does not work when
  cross-compiling. Turn `LIBUVC_INSTALL` off, as libuvc's install rules
  cannot refer to such a target.
- Set `LIBUVC_PACKETS_PER_TRANSFER_MAX` to 8 or so: phones often refuse
  libuvc's default isochronous transfer size with `ENOMEM` (even a Pixel 8,
  now and then), losing frames or all of them. Smaller transfers cost CPU,
  though: streaming 640x480 YUYV on that Pixel 8, the thread handling USB
  events uses about 30% of a core at 8, and 17% at 32.
- libuvc only decodes MJPEG when built with libjpeg. On Android 11 and later,
  `AImageDecoder` from the NDK does it with no library to ship, including the
  usual UVC frames without Huffman tables; see
  [`convert.c`](app/src/main/cpp/convert.c).

### Getting the device

An app cannot open `/dev/bus/usb` on Android, so libusb cannot list or open
devices. Everything goes through `UsbManager`, in Kotlin or Java
([`MainActivity.kt`](app/src/main/java/io/github/libuvc/example/MainActivity.kt)):

- Request the `CAMERA` runtime permission first. Since Android 9, USB
  permission for a device with a video interface is only granted to apps
  holding it: `UsbManager.requestPermission()` otherwise fails without
  asking the user. Declare `android.hardware.camera` as not required, or
  Google Play hides the app from devices without a built-in camera.
- Ask for USB permission with `UsbManager.requestPermission()`. Its
  `PendingIntent` must be mutable, as the system fills in the result, and
  hence explicit (`setPackage()`) since Android 14.
- A `USB_DEVICE_ATTACHED` intent filter with
  [`<usb-device class="14" />`](app/src/main/res/xml/device_filter.xml)
  offers the app when a camera is plugged in, with the USB permission
  included if the user accepts. The class also matches interfaces: webcams
  are usually composite devices, with class 239 rather than 14.
- Get names from `UsbDevice`: `uvc_get_device_descriptor()` reads strings
  through a second device handle, which libusb cannot open on Android.

### Handing it to libuvc

([`uvc_jni.c`](app/src/main/cpp/uvc_jni.c))

- Set `LIBUSB_OPTION_NO_DEVICE_DISCOVERY` before any libusb context exists:
  `libusb_set_option(NULL, LIBUSB_OPTION_NO_DEVICE_DISCOVERY)`. Otherwise
  libusb tries to scan the devices and watch for new ones, which an app may
  not do.
- `uvc_init(&ctx, NULL)` makes libuvc create its own libusb context and run
  the thread that handles its events. With a context of your own instead,
  your code has to call `libusb_handle_events()` itself.
- `UsbManager.openDevice()` gives a `UsbDeviceConnection`; pass its
  `getFileDescriptor()` to `uvc_wrap()`. The file descriptor stays owned by
  the connection, so keep the connection open (and referenced) until
  `uvc_close()` has returned, then close it.
- `uvc_find_device()`, `uvc_get_device_list()` and `uvc_open()` cannot work:
  `uvc_wrap()` replaces them.
- If the kernel's `uvcvideo` driver has the camera, as on phones that
  support USB webcams themselves, libuvc detaches it, and gives the camera
  back to it on `uvc_close()`.

### Threads

- libuvc calls the frame callback on a thread of its own. Drawing straight
  into an `ANativeWindow` from there needs no call into Java at all.
- Calls that talk to the camera, `uvc_wrap()`, starting and stopping a
  stream and `uvc_close()`, block on USB transfers, and libuvc's control
  requests have no timeout: keep them off the main thread.
- `uvc_stop_streaming()` waits for the frame callback to return. Never call
  it from the callback, nor while holding a lock the callback takes.
- A `Surface` must not be drawn on once `surfaceDestroyed()` has returned:
  take it away from the callback synchronously, under a lock.
- When `ACTION_USB_DEVICE_DETACHED` arrives, stop and close the camera:
  libuvc does not report the unplug, the frames just stop coming.
