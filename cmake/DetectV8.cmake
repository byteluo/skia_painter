if(NOT V8_GN_BUILD_DIR AND DEFINED ENV{V8_GN_BUILD_DIR})
  set(V8_GN_BUILD_DIR "$ENV{V8_GN_BUILD_DIR}")
endif()
if(NOT V8_GN_BUILD_DIR)
  foreach(_candidate
      "${CMAKE_CURRENT_SOURCE_DIR}/build/v8-build/v8/out.gn/x64.release"
      "${CMAKE_CURRENT_SOURCE_DIR}/build/v8-build/v8/out.gn/arm64.release")
    if(EXISTS "${_candidate}/args.gn")
      set(V8_GN_BUILD_DIR "${_candidate}")
      break()
    endif()
  endforeach()
endif()

set(_v8_gn_config_found FALSE)
set(_v8_gn_definitions)
if(V8_GN_BUILD_DIR AND EXISTS "${V8_GN_BUILD_DIR}/args.gn")
  get_filename_component(_v8_gn_parent "${V8_GN_BUILD_DIR}" DIRECTORY)
  get_filename_component(_v8_source_dir "${_v8_gn_parent}" DIRECTORY)
  find_program(_v8_gn_executable
    NAMES gn
    PATHS
      "${_v8_source_dir}/buildtools/linux64"
      "${_v8_source_dir}/buildtools/mac"
      "${_v8_source_dir}/buildtools/mac-arm64"
    NO_DEFAULT_PATH
  )
  if(_v8_gn_executable)
    execute_process(
      COMMAND "${_v8_gn_executable}"
              "--root=${_v8_source_dir}"
              desc "${V8_GN_BUILD_DIR}" //:v8_base_without_compiler defines
      RESULT_VARIABLE _v8_gn_result
      OUTPUT_VARIABLE _v8_gn_output
      ERROR_VARIABLE _v8_gn_error
    )
    if(_v8_gn_result EQUAL 0)
      set(_v8_gn_config_found TRUE)
      foreach(_definition
          V8_COMPRESS_POINTERS
          V8_COMPRESS_POINTERS_IN_SHARED_CAGE
          V8_31BIT_SMIS_ON_64BIT_ARCH
          V8_ENABLE_SANDBOX
          CPPGC_POINTER_COMPRESSION)
        if(_v8_gn_output MATCHES "(^|\n)${_definition}(\n|$)")
          list(APPEND _v8_gn_definitions "${_definition}")
        endif()
      endforeach()
    else()
      message(WARNING
        "Could not query V8 GN definitions from ${V8_GN_BUILD_DIR}: "
        "${_v8_gn_error}")
    endif()
  endif()
endif()

if(_v8_gn_config_found)
  set(_v8_expected_definitions "${_v8_gn_definitions}")
  if(V8_COMPILE_DEFINITIONS)
    set(_v8_override_definitions ${V8_COMPILE_DEFINITIONS})
    list(SORT _v8_expected_definitions)
    list(SORT _v8_override_definitions)
    if(NOT _v8_expected_definitions STREQUAL _v8_override_definitions)
      message(FATAL_ERROR
        "V8_COMPILE_DEFINITIONS does not match V8 GN output. "
        "Detected '${_v8_gn_definitions}', received "
        "'${V8_COMPILE_DEFINITIONS}'. Remove the override or fix the V8 build.")
    endif()
  endif()
  string(JOIN "," _v8_candidate ${_v8_expected_definitions})
  if(NOT _v8_candidate)
    set(_v8_candidate "__none__")
  endif()
  set(_v8_candidate_source "V8 GN output at ${V8_GN_BUILD_DIR}")
elseif(V8_COMPILE_DEFINITIONS)
  string(REPLACE ";" "," _v8_candidate "${V8_COMPILE_DEFINITIONS}")
  set(_v8_candidate_source "V8_COMPILE_DEFINITIONS override")
else()
  set(_v8_candidates
    "V8_COMPRESS_POINTERS,V8_COMPRESS_POINTERS_IN_SHARED_CAGE,V8_31BIT_SMIS_ON_64BIT_ARCH,V8_ENABLE_SANDBOX,CPPGC_POINTER_COMPRESSION"
    "V8_COMPRESS_POINTERS,V8_COMPRESS_POINTERS_IN_SHARED_CAGE,V8_31BIT_SMIS_ON_64BIT_ARCH"
    "V8_COMPRESS_POINTERS,V8_COMPRESS_POINTERS_IN_SHARED_CAGE"
    "V8_COMPRESS_POINTERS"
    "__none__"
  )
  set(_v8_candidate_source "V8 ABI probe")
endif()

set(_v8_link_options)
foreach(_directory IN LISTS V8_LINK_DIRECTORIES)
  list(APPEND _v8_link_options "-L${_directory}" "-Wl,-rpath,${_directory}")
endforeach()

set(_v8_had_icu_environment FALSE)
if(DEFINED ENV{V8_ICU_DATA_DIR})
  set(_v8_original_icu_data_dir "$ENV{V8_ICU_DATA_DIR}")
  set(_v8_had_icu_environment TRUE)
endif()

set(_v8_probe_icu_dir)
if(_v8_had_icu_environment AND
   EXISTS "${_v8_original_icu_data_dir}/icudtl.dat")
  set(_v8_probe_icu_dir "${_v8_original_icu_data_dir}")
else()
  set(_v8_icu_candidates)
  if(DEFINED ENV{V8_ROOT})
    list(APPEND _v8_icu_candidates
      "$ENV{V8_ROOT}/share/v8"
      "$ENV{V8_ROOT}/lib"
      "$ENV{V8_ROOT}/libexec"
    )
  endif()
  foreach(_directory IN LISTS V8_LINK_DIRECTORIES V8_INCLUDE_DIRS)
    list(APPEND _v8_icu_candidates
      "${_directory}"
      "${_directory}/../share/v8"
      "${_directory}/../libexec"
    )
  endforeach()
  list(APPEND _v8_icu_candidates
    "/opt/homebrew/opt/v8/share/v8"
    "/opt/homebrew/opt/v8/libexec"
    "/opt/homebrew/opt/v8/lib"
  )
  list(REMOVE_DUPLICATES _v8_icu_candidates)
  foreach(_candidate IN LISTS _v8_icu_candidates)
    if(EXISTS "${_candidate}/icudtl.dat")
      set(_v8_probe_icu_dir "${_candidate}")
      break()
    endif()
  endforeach()
endif()

if(NOT _v8_probe_icu_dir)
  message(FATAL_ERROR
    "The V8 ABI probe needs icudtl.dat. Set V8_ICU_DATA_DIR to its directory "
    "or install it under V8_ROOT or one of V8_LINK_DIRECTORIES.")
endif()
if(_v8_had_icu_environment AND
   NOT "${_v8_original_icu_data_dir}" STREQUAL "${_v8_probe_icu_dir}")
  message(WARNING
    "V8_ICU_DATA_DIR='${_v8_original_icu_data_dir}' does not contain icudtl.dat; "
    "using '${_v8_probe_icu_dir}' for the V8 ABI probe.")
endif()
set(ENV{V8_ICU_DATA_DIR} "${_v8_probe_icu_dir}")

if(DEFINED _v8_candidates)
  set(_v8_candidates_to_try ${_v8_candidates})
else()
  set(_v8_candidates_to_try "${_v8_candidate}")
endif()

set(_v8_config_validated FALSE)
set(_v8_probe_failures)
set(_v8_candidate_index 0)
foreach(_candidate IN LISTS _v8_candidates_to_try)
  math(EXPR _v8_candidate_index "${_v8_candidate_index} + 1")
  if(_candidate STREQUAL "__none__")
    set(_v8_candidate_definitions)
  else()
    string(REPLACE "," ";" _v8_candidate_definitions "${_candidate}")
  endif()

  set(_v8_run_var "_V8_ABI_RUN_${_v8_candidate_index}")
  set(_v8_compile_var "_V8_ABI_COMPILE_${_v8_candidate_index}")
  unset(${_v8_run_var} CACHE)
  unset(${_v8_compile_var} CACHE)
  set(_v8_try_args
    CMAKE_FLAGS
      "-DCMAKE_CXX_STANDARD:STRING=20"
      "-DINCLUDE_DIRECTORIES:STRING=${V8_INCLUDE_DIRS}"
    LINK_OPTIONS ${_v8_link_options}
    LINK_LIBRARIES ${V8_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
    COMPILE_OUTPUT_VARIABLE _v8_compile_output
    RUN_OUTPUT_VARIABLE _v8_run_output
  )
  if(_v8_candidate_definitions)
    foreach(_definition IN LISTS _v8_candidate_definitions)
      list(APPEND _v8_try_args COMPILE_DEFINITIONS "-D${_definition}")
    endforeach()
  endif()

  try_run(${_v8_run_var} ${_v8_compile_var}
    "${CMAKE_BINARY_DIR}/CMakeFiles/v8-abi-${_v8_candidate_index}"
    "${CMAKE_CURRENT_LIST_DIR}/v8_abi_probe.cc"
    ${_v8_try_args}
  )
  if(${_v8_compile_var} AND "${${_v8_run_var}}" STREQUAL "0")
    set(_v8_config_validated TRUE)
    set(V8_COMPILE_DEFINITIONS "${_v8_candidate_definitions}")
    break()
  endif()
  list(APPEND _v8_probe_failures
    "definitions='${_v8_candidate_definitions}', compile='${${_v8_compile_var}}', "
    "run='${${_v8_run_var}}', output='${_v8_run_output}${_v8_compile_output}'")
endforeach()

if(_v8_had_icu_environment)
  set(ENV{V8_ICU_DATA_DIR} "${_v8_original_icu_data_dir}")
else()
  unset(ENV{V8_ICU_DATA_DIR})
endif()

if(NOT _v8_config_validated)
  message(FATAL_ERROR
    "Could not validate V8 compile definitions against the selected headers "
    "and libraries. GN config: '${V8_GN_BUILD_DIR}'. Probe attempts: "
    "${_v8_probe_failures}. Set V8_GN_BUILD_DIR to the matching V8 build or "
    "provide a verified V8_COMPILE_DEFINITIONS override.")
endif()

message(STATUS
  "Validated V8 compile definitions from ${_v8_candidate_source}: "
  "'${V8_COMPILE_DEFINITIONS}'")
