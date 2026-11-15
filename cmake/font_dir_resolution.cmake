set(MISSING_FONT_DIR "${TEST_OUTPUT}/missing-font-dir")
if(EXISTS "${MISSING_FONT_DIR}")
  message(FATAL_ERROR "test path unexpectedly exists: ${MISSING_FONT_DIR}")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env
          "CANVAS_ENGINE_FONT_DIR=${MISSING_FONT_DIR}"
          "${ENGINE}" run "${SOURCE_ROOT}/examples/smoke.js"
  RESULT_VARIABLE env_result
  OUTPUT_VARIABLE env_output
  ERROR_VARIABLE env_error
)
if(env_result EQUAL 0)
  message(FATAL_ERROR "missing CANVAS_ENGINE_FONT_DIR unexpectedly worked")
endif()
string(FIND "${env_output}${env_error}"
  "font directory does not exist: ${MISSING_FONT_DIR}" error_index)
if(error_index EQUAL -1)
  message(FATAL_ERROR
    "missing font directory error did not name the path: ${env_error}")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env
          "CANVAS_ENGINE_FONT_DIR=${MISSING_FONT_DIR}"
          "${ENGINE}" run --font-dir "${VALID_FONT_DIR}"
          "${SOURCE_ROOT}/examples/smoke.js"
  RESULT_VARIABLE explicit_result
  OUTPUT_VARIABLE explicit_output
  ERROR_VARIABLE explicit_error
)
if(NOT explicit_result EQUAL 0)
  message(FATAL_ERROR
    "--font-dir did not override CANVAS_ENGINE_FONT_DIR: ${explicit_error}")
endif()

execute_process(
  COMMAND "${ENGINE}" run --font-dir "${MISSING_FONT_DIR}" --system-fonts
          "${SOURCE_ROOT}/examples/smoke.js"
  RESULT_VARIABLE system_result
  OUTPUT_VARIABLE system_output
  ERROR_VARIABLE system_error
)
if(NOT system_result EQUAL 0)
  message(FATAL_ERROR
    "--system-fonts did not allow a missing bundle: ${system_error}")
endif()

message("font_dir_resolution ok")
