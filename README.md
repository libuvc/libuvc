`libuvc` is a cross-platform library for USB video devices, built atop `libusb`.
It enables fine-grained control over USB video devices exporting the standard USB Video Class
(UVC) interface, enabling developers to write drivers for previously unsupported devices,
or just access UVC devices in a generic fashion.

## Getting and Building libuvc

Prerequisites: You will need `libusb` and [CMake](http://www.cmake.org/) installed.

To build, you can just run these shell commands:

    git clone https://github.com/libuvc/libuvc
    cd libuvc
    mkdir build
    cd build
    cmake ..
    make && sudo make install

and you're set! If you want to change the build configuration, you can edit `CMakeCache.txt`
in the build directory, or use a CMake GUI to make the desired changes.

There is also `BUILD_EXAMPLE` and `BUILD_TEST` options to enable the compilation of `example` and `uvc_test` programs. To use them, replace the `cmake ..` command above with `cmake .. -DBUILD_TEST=ON -DBUILD_EXAMPLE=ON`.
Then you can start them with `./example` and `./uvc_test` respectively. Note that you need OpenCV to build the later (for displaying image).

Two tuning options control how much data is queued for a stream, and are left at their
defaults when unset:

- `LIBUVC_PACKETS_PER_TRANSFER_MAX` (default 32): maximum number of isochronous packets in
  one USB transfer. If starting a stream fails with `submiturb failed, errno=12` (ENOMEM),
  as is common on Android, lower it, e.g. `cmake .. -DLIBUVC_PACKETS_PER_TRANSFER_MAX=8`.
- `LIBUVC_NUM_TRANSFER_BUFS` (default 100, 20 on macOS): number of transfers queued at once.

## Developing with libuvc

The documentation for `libuvc` can currently be found at https://libuvc.github.io/.

Happy hacking!
