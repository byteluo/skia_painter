include(FindPackageHandleStandardArgs)

set(SKIA_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third_party/skia"
    CACHE PATH "Skia source tree")
get_filename_component(SKIA_ROOT "${SKIA_ROOT}" ABSOLUTE)

if(NOT SKIA_INCLUDE_DIRS)
  if(EXISTS "${SKIA_ROOT}/include/core/SkCanvas.h")
    set(SKIA_INCLUDE_DIRS "${SKIA_ROOT}")
  endif()
endif()

set(_SKIA_LIBRARY_SPECS
  "png_encode|src/encode"
  "png_encode_base|src/encode"
  "encoder_common|src/encode"
  "any_decoder|src/codec"
  "icc_support|src/encode"
)
if(UNIX AND NOT APPLE)
  list(APPEND _SKIA_LIBRARY_SPECS "fontmgr_fontconfig|src/ports")
endif()
list(APPEND _SKIA_LIBRARY_SPECS
  "fontmgr_directory_freetype|src/ports"
  "fontmgr_custom|src/ports"
  "typeface_proxy|src/ports"
  "freetype_support|src/ports"
  "char_to_glyphcache|src/utils"
  "freetype|external/+cpp_modules+freetype"
  "core|src/core"
  "base|src/base"
)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
  list(APPEND _SKIA_LIBRARY_SPECS "ml3|src/opts")
endif()

set(_SKIA_LINK_ITEMS)
set(_SKIA_MISSING_LIBRARIES)
if(SKIA_LIBRARIES)
  set(_SKIA_LINK_ITEMS ${SKIA_LIBRARIES})
else()
  foreach(_spec IN LISTS _SKIA_LIBRARY_SPECS)
    string(REPLACE "|" ";" _parts "${_spec}")
    list(GET _parts 0 _name)
    list(GET _parts 1 _directory)
    string(MAKE_C_IDENTIFIER "${_name}_${_directory}" _identifier)
    set(_variable "SKIA_${_identifier}_LIBRARY")
    unset(${_variable} CACHE)
    set(_saved_library_suffixes "${CMAKE_FIND_LIBRARY_SUFFIXES}")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    find_library(${_variable}
      NAMES "${_name}"
      PATHS "${SKIA_ROOT}/bazel-bin/${_directory}"
      NO_DEFAULT_PATH
    )
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${_saved_library_suffixes}")
    if(NOT "${${_variable}}" STREQUAL "" AND
       NOT "${${_variable}}" MATCHES "-NOTFOUND$")
      list(APPEND _SKIA_LINK_ITEMS "${${_variable}}")
    else()
      list(APPEND _SKIA_MISSING_LIBRARIES
        "${SKIA_ROOT}/bazel-bin/${_directory}/lib${_name}.a")
    endif()
  endforeach()

  file(GLOB _SKIA_SKCMS_PIC_OBJECTS CONFIGURE_DEPENDS
    "${SKIA_ROOT}/bazel-bin/modules/skcms/_objs/*/*.pic.o")
  file(GLOB _SKIA_SKCMS_PLAIN_OBJECTS CONFIGURE_DEPENDS
    "${SKIA_ROOT}/bazel-bin/modules/skcms/_objs/*/*.o")
  list(FILTER _SKIA_SKCMS_PLAIN_OBJECTS EXCLUDE
    REGEX "\\.pic\\.o$")
  set(_SKIA_SKCMS_OBJECTS ${_SKIA_SKCMS_PIC_OBJECTS})
  foreach(_object IN LISTS _SKIA_SKCMS_PLAIN_OBJECTS)
    string(REGEX REPLACE "\\.o$" ".pic.o" _pic_object "${_object}")
    list(FIND _SKIA_SKCMS_PIC_OBJECTS "${_pic_object}" _pic_index)
    if(_pic_index EQUAL -1)
      list(APPEND _SKIA_SKCMS_OBJECTS "${_object}")
    endif()
  endforeach()
  if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    list(FILTER _SKIA_SKCMS_OBJECTS EXCLUDE
      REGEX "/skcms_Transform(Hsw|Skx)/")
  endif()
  list(SORT _SKIA_SKCMS_OBJECTS)
  if(_SKIA_SKCMS_OBJECTS)
    list(APPEND _SKIA_LINK_ITEMS ${_SKIA_SKCMS_OBJECTS})
  else()
    list(APPEND _SKIA_MISSING_LIBRARIES
      "${SKIA_ROOT}/bazel-bin/modules/skcms/_objs/*/*.pic.o or *.o")
  endif()
endif()

if(_SKIA_MISSING_LIBRARIES)
  foreach(_missing_artifact IN LISTS _SKIA_MISSING_LIBRARIES)
    message(STATUS "Missing local Skia artifact: ${_missing_artifact}")
  endforeach()
  message(FATAL_ERROR
    "Skia build outputs are missing under ${SKIA_ROOT}/bazel-bin: "
    "${_SKIA_MISSING_LIBRARIES}. Build the required Skia targets or set "
    "SKIA_LIBRARIES explicitly.")
endif()

find_package_handle_standard_args(Skia
  REQUIRED_VARS SKIA_INCLUDE_DIRS SKIA_ROOT
)

if(Skia_FOUND AND NOT TARGET Skia::Skia)
  add_library(Skia::Skia INTERFACE IMPORTED)
  set_target_properties(Skia::Skia PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SKIA_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${_SKIA_LINK_ITEMS}"
  )
endif()
