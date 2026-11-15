if(NOT DEFINED ENGINE OR NOT DEFINED SOURCE_ROOT OR NOT DEFINED TEST_OUTPUT)
  message(FATAL_ERROR "cli_batch test requires ENGINE, SOURCE_ROOT, and TEST_OUTPUT")
endif()

file(MAKE_DIRECTORY "${TEST_OUTPUT}")
set(output_one "${TEST_OUTPUT}/cli_batch_1.png")
set(output_two "${TEST_OUTPUT}/cli_batch_2.png")
set(animate_output "${TEST_OUTPUT}/cli_batch_animate.png")
set(requests_file "${TEST_OUTPUT}/cli_batch_requests.jsonl")
set(animate_requests_file "${TEST_OUTPUT}/cli_batch_animate_requests.jsonl")
file(REMOVE "${output_one}" "${output_two}" "${animate_output}")
file(TO_CMAKE_PATH "${output_one}" output_one)
file(TO_CMAKE_PATH "${output_two}" output_two)
file(TO_CMAKE_PATH "${animate_output}" animate_output)

set(option_json "{\"xAxis\":{\"type\":\"category\",\"data\":[\"A\",\"B\",\"C\"]},\"yAxis\":{\"type\":\"value\"},\"series\":[{\"type\":\"bar\",\"data\":[2,4,3]}]}")
set(request_one "{\"option\":${option_json},\"width\":320,\"height\":180,\"dpr\":1,\"out\":\"${output_one}\"}")
set(request_two "{\"option\":${option_json},\"width\":320,\"height\":180,\"dpr\":1,\"out\":\"${output_two}\"}")
file(WRITE "${requests_file}" "${request_one}\n${request_two}\n")

execute_process(
  COMMAND "${ENGINE}" batch --epoch-ms 1700000000000 --settle-ms 0
          --png-compression fast
  WORKING_DIRECTORY "${SOURCE_ROOT}"
  INPUT_FILE "${requests_file}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
  TIMEOUT 60
)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "canvas_engine batch failed (${result}): ${stderr}${stdout}")
endif()

string(STRIP "${stdout}" stdout)
string(REPLACE "\n" ";" response_lines "${stdout}")
list(LENGTH response_lines response_count)
if(NOT response_count EQUAL 2)
  message(FATAL_ERROR "batch returned ${response_count} responses instead of 2: ${stdout}")
endif()
foreach(response IN LISTS response_lines)
  if(NOT response MATCHES "\"ok\":true")
    message(FATAL_ERROR "batch request failed: ${response}")
  endif()
endforeach()
foreach(output IN ITEMS "${output_one}" "${output_two}")
  if(NOT EXISTS "${output}")
    message(FATAL_ERROR "batch did not create ${output}")
  endif()
  file(SIZE "${output}" output_size)
  if(output_size LESS 100)
    message(FATAL_ERROR "batch output is unexpectedly small: ${output_size} bytes")
  endif()
endforeach()

set(animate_request "{\"option\":${option_json},\"width\":320,\"height\":180,\"dpr\":1,\"out\":\"${animate_output}\"}")
file(WRITE "${animate_requests_file}" "${animate_request}\n")
execute_process(
  COMMAND "${ENGINE}" batch --animate --epoch-ms 1700000000000 --settle-ms 0
          --png-compression small
  WORKING_DIRECTORY "${SOURCE_ROOT}"
  INPUT_FILE "${animate_requests_file}"
  RESULT_VARIABLE animate_result
  OUTPUT_VARIABLE animate_stdout
  ERROR_VARIABLE animate_stderr
  TIMEOUT 60
)
if(NOT animate_result STREQUAL "0")
  message(FATAL_ERROR
    "animated batch failed (${animate_result}): ${animate_stderr}${animate_stdout}")
endif()
if(NOT animate_stdout MATCHES "\"ok\":true" OR NOT EXISTS "${animate_output}")
  message(FATAL_ERROR "animated batch did not create ${animate_output}: ${animate_stdout}")
endif()

message(STATUS "cli_batch ok")
