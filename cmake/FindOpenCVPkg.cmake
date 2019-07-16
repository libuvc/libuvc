#[==============================================[
FindOpenCVPkg
-----------

Searching OpenCV components on systems which does not have fully installed OpenCV. 
Such systems does not include OpenCVConfig.cmake and OpenCVConfigVersion.cmake files.
Example of such system is Ubuntu 18.04.
This package try to use OpenCV config file first if it exists.

#]==============================================]

if (${CMAKE_FIND_PACKAGE_NAME}_FIND_COMPONENTS)
  set(comps ${${CMAKE_FIND_PACKAGE_NAME}_FIND_COMPONENTS})
else()
  set(comps)
endif()

list(LENGTH comps comps_count)
message(STATUS "Components requested: ${comps}")

# Try to find components using standard OpenCV config package
if(comps_count GREATER 0)
  find_package(OpenCV QUIET
    COMPONENTS ${comps}
  )
  if(OpenCV_FOUND)
    set(OpenCVPkg_FOUND TRUE)
    return()    
  endif()
else()
  find_package(OpenCV QUIET)
  if(OpenCV_FOUND)
    set(OpenCVPkg_FOUND TRUE)
    return()    
  endif()
endif()

if(comps_count EQUAL 0)
  set(err_level)
  if(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
    set(err_level FATAL_ERROR)
  endif()
  if (${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
    set(err_level STATUS)
  endif()
  message(${err_level} "Without standart OpenCV config file you must list components you want")
  return()
endif()

unset(required_comps_not_found)
foreach(comp IN LISTS comps)
  string(REPLACE "opencv_" "" comp_name ${comp})
  set(include_rel_path "opencv2/${comp_name}.hpp" "opencv2/${comp_name}/${comp_name}.hpp")
  find_path(OpenCV_${comp}_INCLUDE_DIR
    NAMES ${include_rel_path}
  )
  find_library(OpenCV_${comp}_LIBRARY
    NAMES ${comp}
  )
  if (${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED_${comp} AND 
      (NOT OpenCV_${comp}_INCLUDE_DIR OR NOT OpenCV_${comp}_LIBRARY))
    list(APPEND required_comps_not_found ${comp})
  else()
    message(STATUS "Found component ${comp}: ${OpenCV_${comp}_LIBRARY}")
  endif()
endforeach()

if(required_comps_not_found)
  message(FATAL_ERROR "Following required components not found: ${required_comps_not_found}")
  return()
endif()

foreach(comp IN LISTS comps)
  if(NOT TARGET ${comp})
    message(STATUS "No target ${comp} yet. Creating it")
    add_library(${comp}
      UNKNOWN IMPORTED
    )
    set_target_properties(${comp} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${OpenCV_${comp}_INCLUDE_DIR}
      IMPORTED_LOCATION ${OpenCV_${comp}_LIBRARY}
    )
  endif()
endforeach()

set(OpenCVPkg_FOUND TRUE)
