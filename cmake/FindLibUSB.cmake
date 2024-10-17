#[==============================================[
FindLibUSB
-----------

Searching libusb-1.0 library and creating imported 
target LibUSB::LibUSB

#]==============================================]

# TODO Append parts for Version compasion and REQUIRED support

# if (MSVC OR MINGW)
#     return()
# endif()

if (NOT TARGET LibUSB::LibUSB)
  find_package(PkgConfig)
  pkg_check_modules(LibUSB REQUIRED IMPORTED_TARGET
    libusb-1.0
  )

  if(LibUSB_FOUND)
    message(STATUS "libusb-1.0 found using pkgconfig")

    if(TARGET PkgConfig::LibUSB)
      set_target_properties(PkgConfig::LibUSB PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      )
    else()
      message(WARNING "Could not found libusb-1.0 library file")
    endif()
    add_library(LibUSB::LibUSB ALIAS PkgConfig::LibUSB)
  endif()
else()
  message(WARNING "libusb-1.0 could not be found using pkgconfig")
endif()
