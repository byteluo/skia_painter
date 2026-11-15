file(MAKE_DIRECTORY "${TEST_OUTPUT}")

set(output "${SOURCE_ROOT}/output/echarts_lines.png")
set(backup "${TEST_OUTPUT}/deterministic_render_backup.png")
set(first_render "${TEST_OUTPUT}/deterministic_render_first.png")
set(second_render "${TEST_OUTPUT}/deterministic_render_second.png")
set(had_output FALSE)
if(EXISTS "${output}")
  configure_file("${output}" "${backup}" COPYONLY)
  set(had_output TRUE)
endif()
file(REMOVE "${output}")

execute_process(
  COMMAND "${ENGINE}" run --epoch-ms 1700000000000
          "${SOURCE_ROOT}/examples/echarts_lines.js"
  WORKING_DIRECTORY "${SOURCE_ROOT}"
  RESULT_VARIABLE first_result
  OUTPUT_VARIABLE first_stdout
  ERROR_VARIABLE first_stderr
)
if("${first_result}" STREQUAL "0" AND EXISTS "${output}")
  configure_file("${output}" "${first_render}" COPYONLY)
else()
  set(first_result_message
      "first render failed (${first_result}): ${first_stdout}${first_stderr}")
endif()

if("${first_result}" STREQUAL "0" AND EXISTS "${output}")
  file(REMOVE "${output}")
  execute_process(
    COMMAND "${ENGINE}" run --epoch-ms 1700000000000
            "${SOURCE_ROOT}/examples/echarts_lines.js"
    WORKING_DIRECTORY "${SOURCE_ROOT}"
    RESULT_VARIABLE second_result
    OUTPUT_VARIABLE second_stdout
    ERROR_VARIABLE second_stderr
  )
  if("${second_result}" STREQUAL "0" AND EXISTS "${output}")
    configure_file("${output}" "${second_render}" COPYONLY)
  else()
    set(second_result_message
        "second render failed (${second_result}): ${second_stdout}${second_stderr}")
  endif()
endif()

if(had_output)
  configure_file("${backup}" "${output}" COPYONLY)
else()
  file(REMOVE "${output}")
endif()

if(DEFINED first_result_message)
  message(FATAL_ERROR "${first_result_message}")
endif()
if(DEFINED second_result_message)
  message(FATAL_ERROR "${second_result_message}")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E compare_files "${first_render}"
          "${second_render}"
  RESULT_VARIABLE compare_result
)
if(NOT "${compare_result}" STREQUAL "0")
  message(FATAL_ERROR "deterministic line renders differ")
endif()
message(STATUS "deterministic_render ok")
