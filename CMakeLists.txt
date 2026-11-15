cmake_minimum_required(VERSION 3.20)

project(skia_canvas_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(V8_INCLUDE_DIRS "" CACHE STRING "Semicolon-separated V8 include directories")
set(V8_LINK_DIRECTORIES "" CACHE STRING "Semicolon-separated V8 library directories")
set(V8_LIBRARIES "" CACHE STRING "Semicolon-separated V8 libraries or targets")
set(V8_COMPILE_DEFINITIONS "" CACHE STRING "Override V8 compile definitions")
set(V8_GN_BUILD_DIR "" CACHE PATH "V8 GN output directory used to detect ABI definitions")

set(SKIA_INCLUDE_DIRS "" CACHE STRING "Semicolon-separated Skia include directories")
set(SKIA_LINK_DIRECTORIES "" CACHE STRING "Semicolon-separated Skia library directories")
set(SKIA_LIBRARIES "" CACHE STRING "Semicolon-separated Skia libraries or targets")

if(APPLE)
  if("${V8_INCLUDE_DIRS}" STREQUAL "" AND EXISTS "/opt/homebrew/opt/v8/include")
    set(V8_INCLUDE_DIRS "/opt/homebrew/opt/v8/include")
  endif()
  if("${V8_LINK_DIRECTORIES}" STREQUAL "" AND EXISTS "/opt/homebrew/opt/v8/lib")
    set(V8_LINK_DIRECTORIES "/opt/homebrew/opt/v8/lib;/opt/homebrew/opt/v8/libexec")
  endif()
  if("${V8_LIBRARIES}" STREQUAL "" AND EXISTS "/opt/homebrew/opt/v8/lib/libv8.dylib")
    set(V8_LIBRARIES "v8;v8_libplatform;v8_libbase")
  endif()
elseif(UNIX)
  # Auto-detect V8 from environment variable or common Linux paths
  if("${V8_INCLUDE_DIRS}" STREQUAL "")
    if(DEFINED ENV{V8_ROOT} AND EXISTS "$ENV{V8_ROOT}/include")
      set(V8_INCLUDE_DIRS "$ENV{V8_ROOT}/include")
    elseif(EXISTS "/usr/include/v8")
      set(V8_INCLUDE_DIRS "/usr/include/v8")
    elseif(EXISTS "/usr/local/include/v8")
      set(V8_INCLUDE_DIRS "/usr/local/include/v8")
    endif()
  endif()
  if("${V8_LINK_DIRECTORIES}" STREQUAL "")
    if(DEFINED ENV{V8_ROOT} AND EXISTS "$ENV{V8_ROOT}/lib")
      set(V8_LINK_DIRECTORIES "$ENV{V8_ROOT}/lib")
    endif()
  endif()
  if("${V8_LIBRARIES}" STREQUAL "")
    if(DEFINED ENV{V8_ROOT} AND EXISTS "$ENV{V8_ROOT}/lib/libv8.so")
      set(V8_LIBRARIES "v8;v8_libplatform;v8_libbase")
    elseif(EXISTS "/usr/lib/x86_64-linux-gnu/libv8.so" OR EXISTS "/usr/lib/libv8.so")
      set(V8_LIBRARIES "v8;v8_libplatform;v8_libbase")
    endif()
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Skia REQUIRED)

foreach(required_var
    V8_INCLUDE_DIRS
    V8_LIBRARIES
    SKIA_INCLUDE_DIRS)
  if("${${required_var}}" STREQUAL "")
    message(FATAL_ERROR
      "${required_var} is empty. Configure this project with explicit V8/Skia include paths and libraries.")
  endif()
endforeach()

find_package(Threads REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
include(cmake/DetectV8.cmake)
include(CTest)
include(GNUInstallDirs)
include(cmake/fonts.cmake)

add_library(canvas_engine_core STATIC
  src/canvas/CanvasSurface.cc
  src/canvas/Canvas2DContext.cc
  src/canvas/FontManager.cc
  src/canvas/ColorParser.cc
  src/canvas/FontParser.cc
  src/canvas/ImageAsset.cc
  src/runtime/EventLoop.cc
  src/runtime/HandleRegistry.cc
  src/runtime/ScriptEngine.cc
  src/runtime/ScriptEngineImpl.cc
  src/runtime/ScriptLoader.cc
  src/runtime/bindings/CanvasBinding.cc
  src/runtime/bindings/Context2DBinding.cc
  src/runtime/bindings/Context2DImages.cc
  src/runtime/bindings/Context2DText.cc
  src/runtime/bindings/GlobalBindings.cc
  src/runtime/bindings/GradientBinding.cc
  src/runtime/bindings/ImageBinding.cc
  src/runtime/bindings/Path2DBinding.cc
  src/runtime/bindings/PatternBinding.cc
  src/runtime/V8Platform.cc
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/js/echarts_host.js"
     ECHARTS_HOST_SOURCE)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/echarts_host.cc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/echarts_host.cc"
  @ONLY
)
target_sources(canvas_engine_core PRIVATE
  src/echarts/EChartsRenderer.cc
  "${CMAKE_CURRENT_BINARY_DIR}/generated/echarts_host.cc"
)

target_include_directories(canvas_engine_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${V8_INCLUDE_DIRS}
    ${SKIA_INCLUDE_DIRS}
)

if(NOT "${V8_COMPILE_DEFINITIONS}" STREQUAL "")
  target_compile_definitions(canvas_engine_core
    PUBLIC ${V8_COMPILE_DEFINITIONS})
endif()

if(NOT "${V8_LINK_DIRECTORIES}" STREQUAL "")
  target_link_directories(canvas_engine_core
    PUBLIC ${V8_LINK_DIRECTORIES})
endif()

if(NOT "${SKIA_LINK_DIRECTORIES}" STREQUAL "")
  target_link_directories(canvas_engine_core
    PUBLIC ${SKIA_LINK_DIRECTORIES})
endif()

target_compile_definitions(canvas_engine_core PRIVATE
  "CANVAS_ENGINE_DEFAULT_FONT_DIR=\"${CANVAS_ENGINE_FONTS_DIR}\"")
if(CANVAS_ENGINE_BUNDLED_FONTS)
  target_compile_definitions(canvas_engine_core PRIVATE
    CANVAS_ENGINE_BUNDLED_FONTS=1)
else()
  target_compile_definitions(canvas_engine_core PRIVATE
    CANVAS_ENGINE_BUNDLED_FONTS=0)
endif()

target_link_libraries(canvas_engine_core
  PUBLIC
    ${V8_LIBRARIES}
    Skia::Skia
    Threads::Threads
  PRIVATE
    ZLIB::ZLIB
    PNG::PNG
)
if(UNIX AND NOT APPLE)
  target_link_libraries(canvas_engine_core PRIVATE fontconfig)
endif()

add_executable(canvas_engine src/main.cc src/cli/Cli.cc)
target_link_libraries(canvas_engine PRIVATE canvas_engine_core)
if(CANVAS_ENGINE_BUNDLED_FONTS)
  target_compile_definitions(canvas_engine PRIVATE
    CANVAS_ENGINE_BUNDLED_FONTS=1)
else()
  target_compile_definitions(canvas_engine PRIVATE
    CANVAS_ENGINE_BUNDLED_FONTS=0)
endif()

add_executable(color_parser_test tests/color_parser_test.cc)
target_link_libraries(color_parser_test PRIVATE canvas_engine_core)

add_executable(font_parser_test tests/font_parser_test.cc)
target_link_libraries(font_parser_test PRIVATE canvas_engine_core)

add_executable(multi_engine_test tests/multi_engine_test.cc)
target_link_libraries(multi_engine_test PRIVATE canvas_engine_core)

add_executable(handle_registry_test tests/handle_registry_test.cc)
target_include_directories(handle_registry_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime
)
target_link_libraries(handle_registry_test PRIVATE canvas_engine_core)

function(canvas_engine_apply_v8_rpath target)
  if(NOT "${V8_LINK_DIRECTORIES}" STREQUAL "")
    set_target_properties(${target} PROPERTIES
      BUILD_RPATH "${V8_LINK_DIRECTORIES}"
      INSTALL_RPATH "${V8_LINK_DIRECTORIES}"
    )
  endif()
endfunction()

canvas_engine_apply_v8_rpath(canvas_engine)
canvas_engine_apply_v8_rpath(color_parser_test)
canvas_engine_apply_v8_rpath(font_parser_test)
canvas_engine_apply_v8_rpath(multi_engine_test)
canvas_engine_apply_v8_rpath(handle_registry_test)

if(APPLE)
  target_link_libraries(canvas_engine_core
    PRIVATE
      "-framework CoreFoundation"
      "-framework CoreGraphics"
      "-framework ImageIO"
      "-framework CoreServices"
  )
elseif(UNIX)
  target_include_directories(canvas_engine_core PRIVATE ${PNG_INCLUDE_DIRS})
  target_link_libraries(canvas_engine_core PRIVATE dl m)
endif()

install(TARGETS canvas_engine
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(CANVAS_ENGINE_BUNDLED_FONTS)
  install(DIRECTORY "${CANVAS_ENGINE_FONTS_DIR}/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/fonts")
endif()

if(BUILD_TESTING)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_output")
  set(RENDER_REGRESSION_STAMP_PNG
      "${CMAKE_CURRENT_BINARY_DIR}/test_output/regression_stamp.png")
  set(RENDER_REGRESSION_OUTPUT_PNG
      "${CMAKE_CURRENT_BINARY_DIR}/test_output/render_regression.png")
  set(TEXT_ROTATE_REGRESSION_OUTPUT_PNG
      "${CMAKE_CURRENT_BINARY_DIR}/test_output/text_rotate_regression.png")
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/render_regression.js.in
    ${CMAKE_CURRENT_BINARY_DIR}/render_regression.js
    @ONLY
  )
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/text_rotate_regression.js.in
    ${CMAKE_CURRENT_BINARY_DIR}/text_rotate_regression.js
    @ONLY
  )
  set(TEXT_CJK_VISUAL_OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/test_output/text_cjk_visual.png")
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/text_cjk_visual.js.in
    ${CMAKE_CURRENT_BINARY_DIR}/text_cjk_visual.js
    @ONLY
  )

  add_test(
    NAME smoke
    COMMAND canvas_engine ${CMAKE_CURRENT_SOURCE_DIR}/examples/smoke.js
  )
  set_tests_properties(smoke PROPERTIES
    PASS_REGULAR_EXPRESSION "smoke"
  )

  set(V8_ICU_DATA_MISSING_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/test_output/missing-v8-data")
  add_test(
    NAME v8_data_dir_fallback
    COMMAND ${CMAKE_COMMAND} -E env
            "V8_ICU_DATA_DIR=${V8_ICU_DATA_MISSING_DIR}"
            $<TARGET_FILE:canvas_engine>
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/smoke.js
  )
  set_tests_properties(v8_data_dir_fallback PROPERTIES
    PASS_REGULAR_EXPRESSION
      "V8_ICU_DATA_DIR='${V8_ICU_DATA_MISSING_DIR}'.*icudtl.dat"
  )

  add_test(
    NAME handle_gc
    COMMAND $<TARGET_FILE:canvas_engine>
            --expose-gc
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/handle_gc.js
  )
  set_tests_properties(handle_gc PROPERTIES
    PASS_REGULAR_EXPRESSION "true\ncanvases=[0-9]+"
  )

  add_test(
    NAME color_parser
    COMMAND color_parser_test
  )

  add_test(
    NAME handle_registry
    COMMAND handle_registry_test
  )

  add_test(
    NAME multi_engine
    COMMAND multi_engine_test
  )

  add_test(
    NAME font_parser
    COMMAND font_parser_test
  )

  add_test(
    NAME font_weight_regression
    COMMAND $<TARGET_FILE:canvas_engine>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/font_weight_regression.js
  )
  set_tests_properties(font_weight_regression PROPERTIES
    PASS_REGULAR_EXPRESSION "font_weight_regression ok"
  )

  if(CANVAS_ENGINE_BUNDLED_FONTS)
    add_test(
      NAME text_metrics
      COMMAND $<TARGET_FILE:canvas_engine>
              ${CMAKE_CURRENT_SOURCE_DIR}/tests/text_metrics.js
    )
    set_tests_properties(text_metrics PROPERTIES
      PASS_REGULAR_EXPRESSION "text_metrics ok"
    )

    add_test(
      NAME text_cjk
      COMMAND $<TARGET_FILE:canvas_engine>
              ${CMAKE_CURRENT_SOURCE_DIR}/tests/text_cjk.js
    )
    set_tests_properties(text_cjk PROPERTIES
      PASS_REGULAR_EXPRESSION "text_cjk ok"
    )

    add_test(
      NAME font_dir_resolution
      COMMAND ${CMAKE_COMMAND}
              -DENGINE=$<TARGET_FILE:canvas_engine>
              -DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
              -DTEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test_output
              -DVALID_FONT_DIR=${CANVAS_ENGINE_FONTS_DIR}
              -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/font_dir_resolution.cmake
    )
    set_tests_properties(font_dir_resolution PROPERTIES
      PASS_REGULAR_EXPRESSION "font_dir_resolution ok"
    )
  endif()

  add_test(
    NAME canvas_semantics
    COMMAND $<TARGET_FILE:canvas_engine>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/canvas_semantics.js
  )
  set_tests_properties(canvas_semantics PROPERTIES
    PASS_REGULAR_EXPRESSION "canvas_semantics ok"
  )
  add_test(
    NAME path2d
    COMMAND $<TARGET_FILE:canvas_engine>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/path2d.js
  )
  set_tests_properties(path2d PROPERTIES
    PASS_REGULAR_EXPRESSION "path2d ok"
  )

  add_test(
    NAME image_loading
    COMMAND $<TARGET_FILE:canvas_engine>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/image_loading.js
  )
  set_tests_properties(image_loading PROPERTIES
    PASS_REGULAR_EXPRESSION "image_loading ok"
  )

  add_test(
    NAME task_queue
    COMMAND $<TARGET_FILE:canvas_engine>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/task_queue.js
  )
  set_tests_properties(task_queue PROPERTIES
    PASS_REGULAR_EXPRESSION "task_queue ok"
    FAIL_REGULAR_EXPRESSION "DrainPendingTasks hit the 10000ms virtual advance cap"
  )

  add_test(
    NAME drawing_animation_frame
    COMMAND $<TARGET_FILE:canvas_engine>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/drawing_animation_frame.js
  )
  set_tests_properties(drawing_animation_frame PROPERTIES
    PASS_REGULAR_EXPRESSION "DrainPendingTasks hit the 10000ms virtual advance cap"
  )

  add_test(
    NAME deterministic_render
    COMMAND ${CMAKE_COMMAND}
            -DENGINE=$<TARGET_FILE:canvas_engine>
            -DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -DTEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test_output
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/deterministic_render.cmake
  )
  set_tests_properties(deterministic_render PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RUN_SERIAL TRUE
    PASS_REGULAR_EXPRESSION "deterministic_render ok"
  )

  add_test(
    NAME clock_epoch
    COMMAND ${CMAKE_COMMAND} -E env
            CANVAS_ENGINE_EPOCH_MS=1700000000000
            $<TARGET_FILE:canvas_engine>
            run ${CMAKE_CURRENT_SOURCE_DIR}/tests/clock_epoch.js
  )
  set_tests_properties(clock_epoch PROPERTIES
    PASS_REGULAR_EXPRESSION "clock_epoch ok"
  )

  add_test(
    NAME realtime_clock
    COMMAND $<TARGET_FILE:canvas_engine>
            run --realtime-clock --epoch-ms 1700000000000
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/realtime_clock.js
  )
  set_tests_properties(realtime_clock PROPERTIES
    PASS_REGULAR_EXPRESSION "realtime_clock ok"
    TIMEOUT 5
  )

  add_test(
    NAME render_regression_render
    COMMAND $<TARGET_FILE:canvas_engine>
            ${CMAKE_CURRENT_BINARY_DIR}/render_regression.js
  )
  set_tests_properties(render_regression_render PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PASS_REGULAR_EXPRESSION "wrote ${CMAKE_CURRENT_BINARY_DIR}/test_output/render_regression\\.png"
  )

  add_test(
    NAME render_regression_verify
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/golden_test.py
            --actual
            "${CMAKE_CURRENT_BINARY_DIR}/test_output/render_regression.png"
            --golden
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/render_regression.png"
            --config
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/tolerances.json"
            --diff
            "${CMAKE_CURRENT_BINARY_DIR}/golden-diffs/render_regression.png"
  )
  set_tests_properties(render_regression_verify PROPERTIES
    DEPENDS render_regression_render
  )

  add_test(
    NAME text_rotate_regression_render
    COMMAND $<TARGET_FILE:canvas_engine>
            ${CMAKE_CURRENT_BINARY_DIR}/text_rotate_regression.js
  )
  set_tests_properties(text_rotate_regression_render PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PASS_REGULAR_EXPRESSION
      "wrote ${CMAKE_CURRENT_BINARY_DIR}/test_output/text_rotate_regression\\.png"
  )

  add_test(
    NAME text_rotate_regression_verify
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/golden_test.py
            --actual
            "${CMAKE_CURRENT_BINARY_DIR}/test_output/text_rotate_regression.png"
            --golden
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/text_rotate_regression.png"
            --config
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/tolerances.json"
            --diff
            "${CMAKE_CURRENT_BINARY_DIR}/golden-diffs/text_rotate_regression.png"
  )
  set_tests_properties(text_rotate_regression_verify PROPERTIES
    DEPENDS text_rotate_regression_render
  )

  add_test(
    NAME compare_coverage_verify
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_compare_coverage.py
  )
  set_tests_properties(compare_coverage_verify PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  set(ECHARTS_DIST_PATH
      "${CMAKE_CURRENT_SOURCE_DIR}/node_modules/echarts/dist/echarts.js")
  if(EXISTS "${ECHARTS_DIST_PATH}")
    file(GLOB ECHARTS_EXAMPLE_SCRIPTS CONFIGURE_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/echarts_*.js")
    foreach(example_script IN LISTS ECHARTS_EXAMPLE_SCRIPTS)
      get_filename_component(example_name "${example_script}" NAME_WE)
      add_test(
        NAME "golden_${example_name}"
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/scripts/golden_test.py
                --engine "$<TARGET_FILE:canvas_engine>"
                --script "${example_script}"
                --run-dir
                "${CMAKE_CURRENT_BINARY_DIR}/golden-runs/${example_name}"
                --output-name "${example_name}.png"
                --golden
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/${example_name}.png"
                --config
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/tolerances.json"
                --diff
                "${CMAKE_CURRENT_BINARY_DIR}/golden-diffs/${example_name}.png"
      )
    endforeach()

    add_test(
      NAME cli_render
      COMMAND ${CMAKE_COMMAND}
              -DENGINE=$<TARGET_FILE:canvas_engine>
              -DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
              -DTEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test_output
              -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cli_render.cmake
    )
    set_tests_properties(cli_render PROPERTIES
      PASS_REGULAR_EXPRESSION "cli_render ok"
    )

    add_test(
      NAME cli_batch
      COMMAND ${CMAKE_COMMAND}
              -DENGINE=$<TARGET_FILE:canvas_engine>
              -DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
              -DTEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test_output
              -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cli_batch.cmake
    )
    set_tests_properties(cli_batch PROPERTIES
      PASS_REGULAR_EXPRESSION "cli_batch ok"
    )

    set(ECHARTS_SUNBURST_REGRESSION_OUTPUT_PNG
        "${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_sunburst_regression.png")
    set(ECHARTS_TIMELINE_REGRESSION_OUTPUT_PNG
        "${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_timeline_regression.png")
    set(ECHARTS_CUSTOM_SERIES_REGRESSION_OUTPUT_PNG
        "${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_custom_series_regression.png")
    set(ECHARTS_PICTORIAL_BAR_REGRESSION_OUTPUT_PNG
        "${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_pictorial_bar_regression.png")
    set(ECHARTS_GAUGE_REGRESSION_OUTPUT_PNG
        "${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_gauge_regression.png")
    set(ECHARTS_IMAGE_SCATTER_MARKER_OUTPUT_PNG
        "${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_image_scatter_marker.png")
    set(ECHARTS_IMAGE_SCATTER_REGRESSION_OUTPUT_PNG
        "${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_image_scatter_regression.png")
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/echarts_regression.js.in
      ${CMAKE_CURRENT_BINARY_DIR}/echarts_regression.js
      @ONLY
    )

    add_test(
      NAME echarts_regression_render
      COMMAND $<TARGET_FILE:canvas_engine>
              ${CMAKE_CURRENT_BINARY_DIR}/echarts_regression.js
    )
    set_tests_properties(echarts_regression_render PROPERTIES
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      PASS_REGULAR_EXPRESSION
        "wrote ${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_image_scatter_regression\\.png"
    )

    foreach(regression_name IN ITEMS
        sunburst
        timeline
        custom_series
        pictorial_bar
        gauge
        image_scatter)
      set(regression_image "echarts_${regression_name}_regression.png")
      add_test(
        NAME "golden_${regression_image}"
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/scripts/golden_test.py
                --actual
                "${CMAKE_CURRENT_BINARY_DIR}/test_output/${regression_image}"
                --golden
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/${regression_image}"
                --config
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/tolerances.json"
                --diff
                "${CMAKE_CURRENT_BINARY_DIR}/golden-diffs/${regression_image}"
      )
      set_tests_properties("golden_${regression_image}" PROPERTIES
        DEPENDS echarts_regression_render
      )
    endforeach()

  endif()
endif()
