if(NOT DEFINED ENGINE OR NOT DEFINED SOURCE_ROOT OR NOT DEFINED TEST_OUTPUT)
  message(FATAL_ERROR "cli_render test requires ENGINE, SOURCE_ROOT, and TEST_OUTPUT")
endif()

file(MAKE_DIRECTORY "${TEST_OUTPUT}")
set(output "${TEST_OUTPUT}/cli_render.png")
set(animate_output "${TEST_OUTPUT}/cli_render_animate.png")
set(fast_output "${TEST_OUTPUT}/cli_render_fast.png")
set(small_output "${TEST_OUTPUT}/cli_render_small.png")
file(REMOVE "${output}" "${animate_output}" "${fast_output}" "${small_output}")

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

foreach(compression IN ITEMS fast small)
  set(compressed_output "${TEST_OUTPUT}/cli_render_${compression}.png")
  execute_process(
    COMMAND "${ENGINE}" render
            --png-compression "${compression}"
            --epoch-ms 1700000000000
            --settle-ms 0
            --width 320
            --height 180
            --dpr 1
            -o "${compressed_output}"
            "${SOURCE_ROOT}/tests/cli_render_option.json"
    WORKING_DIRECTORY "${SOURCE_ROOT}"
    RESULT_VARIABLE compression_result
    OUTPUT_VARIABLE compression_stdout
    ERROR_VARIABLE compression_stderr
    TIMEOUT 60
  )
  if(NOT compression_result STREQUAL "0" OR NOT EXISTS "${compressed_output}")
    message(FATAL_ERROR
      "render with ${compression} compression failed (${compression_result}): ${compression_stderr}${compression_stdout}")
  endif()
endforeach()

execute_process(
  COMMAND "${ENGINE}" render
          --animate
          --epoch-ms 1700000000000
          --settle-ms 0
          --width 320
          --height 180
          --dpr 1
          -o "${animate_output}"
          "${SOURCE_ROOT}/tests/cli_render_option.json"
  WORKING_DIRECTORY "${SOURCE_ROOT}"
  RESULT_VARIABLE animate_result
  OUTPUT_VARIABLE animate_stdout
  ERROR_VARIABLE animate_stderr
  TIMEOUT 60
)
if(NOT animate_result STREQUAL "0")
  message(FATAL_ERROR
    "animated render failed (${animate_result}): ${animate_stderr}${animate_stdout}")
endif()
if(NOT EXISTS "${animate_output}")
  message(FATAL_ERROR "animated render did not create ${animate_output}")
endif()

message(STATUS "cli_render ok")
