#[==============================================[
FindJpegPkg
-----------

Searching jpeg library library and creating imported
target JPEG::JPEG (this is different from shipped one with CMake)

#]==============================================]

# TODO Append parts for Version compasion and REQUIRED support
if (MSVC OR MINGW)
  return()
endif()

# Try to find JPEG using a module or pkg-config. If that doesn't work, search for the header.
if (NOT TARGET JPEG::JPEG)
  find_package(JPEG)
  if (JPEG_FOUND)
    message(STATUS "Found JPEG library using standard module")
    set(IMPORTED_JPEG_INCLUDE_DIRS ${JPEG_INCLUDE_DIR})
    set(IMPORTED_JPEG_LIBRARIES ${JPEG_LIBRARIES})
  else()
    find_package(PkgConfig)
    pkg_check_modules(JPEG libjpeg)
    if(JPEG_FOUND)
      message(STATUS "JPEG found by pkgconfig")

      if (DEFINED JPEG_INCLUDE_DIRS)
        set(IMPORTED_JPEG_INCLUDE_DIRS ${JPEG_INCLUDE_DIRS})
      endif()

      if(JPEG_LIBRARIES)
        find_library(JPEG_LIBRARY
          NAMES ${JPEG_LIBRARIES}
          PATHS ${JPEG_LIBDIR} ${JPEG_LIBRARY_DIRS}
        )
        if(JPEG_LIBRARY)
          set(IMPORTED_JPEG_LIBRARIES ${JPEG_LIBRARY})
        else()
          message(WARNING "Could not found libjpeg library file")
        endif()
      endif()
    else()
      message(STATUS "Searching library manually")
      find_path(IMPORTED_JPEG_INCLUDE_DIRS
        NAMES jpeglib.h
      )

      if (NOT IMPORTED_JPEG_LIBRARIES)
        find_library(IMPORTED_JPEG_LIBRARIES
            NAMES jpeg
        )
      endif()
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(JPEG
      REQUIRED_VARS IMPORTED_JPEG_LIBRARIES IMPORTED_JPEG_INCLUDE_DIRS
  )

  if(JPEG_FOUND)
    set(JpegPkg_FOUND TRUE)
    if (NOT TARGET JPEG::JPEG)
      add_library(JPEG::JPEG
        UNKNOWN IMPORTED
      )
      if(IMPORTED_JPEG_INCLUDE_DIRS)
        set_target_properties(JPEG::JPEG PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES ${IMPORTED_JPEG_INCLUDE_DIRS}
        )
      endif()
      if(IMPORTED_JPEG_LIBRARIES)
        set_target_properties(JPEG::JPEG PROPERTIES
          IMPORTED_LINK_INTERFACE_LANGUAGES "C" 
          IMPORTED_LOCATION ${IMPORTED_JPEG_LIBRARIES}
        )
      endif()
    endif()
  endif()

endif()
