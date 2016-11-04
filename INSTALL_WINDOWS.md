## Prerequisites


1. [Libusb with isochronous support] (https://github.com/pupil-labs/libusb)
2. Cmake  (https://cmake.org/files/v3.7/cmake-3.7.0-rc2-win64-x64.msi)

## Installation

1. Download and extract posix threads for Windows from http://kent.dl.sourceforge.net/project/pthreads4w/pthreads-w32-2-9-1-release.zip
   Add the dll\x64 directory to the system path
2. Run cmake. Set the top level libuvc directory as the source path. Set a
   directory of your choice as the binares location
3. Click configure. Select "Visual studio 14 2015 Win64" as code generator. Fill in the
   locations for LIBUSB (set include to <libusbdir>/libusb and lib to <libusbdir>/x64/Release/dll/libusb-1.0.lib )
   and PTHREAD (include to <pthread_dir>/include, lib to
   <pthread_dir>/lib/x64/pthreadVC2.lib). LIBJPEG is optional. If you are using pyuvc, it provides decoding via turbojpeg.
4. Click configure again. Click generate.
5. Open <bin_dir>\libuvc.sln Using Visual Studio. Make sure x64 platform is
selected. Select Release configuration. Build the "ALL_BUILD" project
6. Add the <bindir>\Release to system path
