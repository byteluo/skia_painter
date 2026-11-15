if(NOT DEFINED ENGINE OR NOT DEFINED SOURCE_ROOT OR NOT DEFINED TEST_OUTPUT)
  message(FATAL_ERROR "cli_render test requires ENGINE, SOURCE_ROOT, and TEST_OUTPUT")
endif()

file(MAKE_DIRECTORY "${TEST_OUTPUT}")
set(output "${TEST_OUTPUT}/cli_render.png")
file(REMOVE "${output}")

execute_process(
  COMMAND "${ENGINE}" render
          --epoch-ms 1700000000000
          --settle-ms 0
          --width 320
          --height 180
          --dpr 1
          -o "${output}"
          "${SOURCE_ROOT}/tests/cli_render_option.json"
  WORKING_DIRECTORY "${SOURCE_ROOT}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
  TIMEOUT 60
)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "canvas_engine render failed (${result}): ${stderr}${stdout}")
endif()
if(NOT EXISTS "${output}")
  message(FATAL_ERROR "canvas_engine render did not create ${output}")
endif()
file(SIZE "${output}" output_size)
if(output_size LESS 100)
  message(FATAL_ERROR "render output is unexpectedly small: ${output_size} bytes")
endif()

message(STATUS "cli_render ok")
