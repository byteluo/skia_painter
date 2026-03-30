cmake_minimum_required(VERSION 3.20)

project(skia_canvas_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(V8_INCLUDE_DIRS "" CACHE STRING "Semicolon-separated V8 include directories")
set(V8_LINK_DIRECTORIES "" CACHE STRING "Semicolon-separated V8 library directories")
set(V8_LIBRARIES "" CACHE STRING "Semicolon-separated V8 libraries or targets")
set(V8_COMPILE_DEFINITIONS "" CACHE STRING "Semicolon-separated V8 compile definitions")

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
  if("${V8_COMPILE_DEFINITIONS}" STREQUAL "" AND EXISTS "/opt/homebrew/opt/v8/include")
    set(V8_COMPILE_DEFINITIONS
      "V8_COMPRESS_POINTERS;V8_COMPRESS_POINTERS_IN_SHARED_CAGE;V8_31BIT_SMIS_ON_64BIT_ARCH;V8_ENABLE_SANDBOX;CPPGC_POINTER_COMPRESSION")
  endif()
endif()

set(SKIA_LOCAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/skia")
set(SKIA_LOCAL_CORE "${SKIA_LOCAL_ROOT}/bazel-bin/src/core/libcore.a")
set(SKIA_LOCAL_BASE "${SKIA_LOCAL_ROOT}/bazel-bin/src/base/libbase.a")
set(SKIA_LOCAL_SKCMS "${SKIA_LOCAL_ROOT}/bazel-bin/modules/skcms/_objs/skcms_public/skcms.o")
set(SKIA_LOCAL_SKCMS_BASELINE
    "${SKIA_LOCAL_ROOT}/bazel-bin/modules/skcms/_objs/skcms_TransformBaseline/skcms_TransformBaseline.o")

if("${SKIA_INCLUDE_DIRS}" STREQUAL "" AND EXISTS "${SKIA_LOCAL_ROOT}/include/core/SkCanvas.h")
  set(SKIA_INCLUDE_DIRS "${SKIA_LOCAL_ROOT}")
endif()

if("${SKIA_LIBRARIES}" STREQUAL "" AND
   EXISTS "${SKIA_LOCAL_CORE}" AND
   EXISTS "${SKIA_LOCAL_BASE}" AND
   EXISTS "${SKIA_LOCAL_SKCMS}" AND
   EXISTS "${SKIA_LOCAL_SKCMS_BASELINE}")
  set(SKIA_LIBRARIES
    "${SKIA_LOCAL_CORE};${SKIA_LOCAL_BASE};${SKIA_LOCAL_SKCMS};${SKIA_LOCAL_SKCMS_BASELINE}")
endif()

foreach(required_var
    V8_INCLUDE_DIRS
    V8_LIBRARIES
    SKIA_INCLUDE_DIRS
    SKIA_LIBRARIES)
  if("${${required_var}}" STREQUAL "")
    message(FATAL_ERROR
      "${required_var} is empty. Configure this project with explicit V8/Skia include paths and libraries.")
  endif()
endforeach()

find_package(Threads REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(CTest)
include(GNUInstallDirs)

add_executable(canvas_engine
  src/main.cc
  src/canvas/CanvasSurface.cc
  src/canvas/Canvas2DContext.cc
  src/canvas/ColorParser.cc
  src/canvas/ImageAsset.cc
  src/runtime/ScriptEngine.cc
)

target_include_directories(canvas_engine
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${V8_INCLUDE_DIRS}
    ${SKIA_INCLUDE_DIRS}
)

if(NOT "${V8_COMPILE_DEFINITIONS}" STREQUAL "")
  target_compile_definitions(canvas_engine PRIVATE ${V8_COMPILE_DEFINITIONS})
endif()

if(NOT "${V8_LINK_DIRECTORIES}" STREQUAL "")
  target_link_directories(canvas_engine PRIVATE ${V8_LINK_DIRECTORIES})
endif()

if(NOT "${SKIA_LINK_DIRECTORIES}" STREQUAL "")
  target_link_directories(canvas_engine PRIVATE ${SKIA_LINK_DIRECTORIES})
endif()

target_link_libraries(canvas_engine
  PRIVATE
    ${V8_LIBRARIES}
    ${SKIA_LIBRARIES}
    Threads::Threads
)

if(NOT "${V8_LINK_DIRECTORIES}" STREQUAL "")
  set_target_properties(canvas_engine PROPERTIES
    BUILD_RPATH "${V8_LINK_DIRECTORIES}"
    INSTALL_RPATH "${V8_LINK_DIRECTORIES}"
  )
endif()

if(APPLE)
  target_link_libraries(canvas_engine
    PRIVATE
      "-framework CoreFoundation"
      "-framework CoreGraphics"
      "-framework ImageIO"
      "-framework CoreText"
      "-framework CoreServices"
  )
elseif(UNIX)
  target_link_libraries(canvas_engine PRIVATE dl m)
endif()

install(TARGETS canvas_engine
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_TESTING)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_output")
  set(RENDER_REGRESSION_STAMP_PNG
      "${CMAKE_CURRENT_BINARY_DIR}/test_output/regression_stamp.png")
  set(RENDER_REGRESSION_OUTPUT_PNG
      "${CMAKE_CURRENT_BINARY_DIR}/test_output/render_regression.png")
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/render_regression.js.in
    ${CMAKE_CURRENT_BINARY_DIR}/render_regression.js
    @ONLY
  )

  add_test(
    NAME smoke
    COMMAND canvas_engine ${CMAKE_CURRENT_SOURCE_DIR}/examples/smoke.js
  )
  set_tests_properties(smoke PROPERTIES
    PASS_REGULAR_EXPRESSION "smoke"
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
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_render_regression.py
            ${CMAKE_CURRENT_BINARY_DIR}/test_output/render_regression.png
  )
  set_tests_properties(render_regression_verify PROPERTIES
    DEPENDS render_regression_render
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

    add_test(
      NAME echarts_regression_verify
      COMMAND ${Python3_EXECUTABLE}
              ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_echarts_regression.py
              ${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_sunburst_regression.png
              ${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_timeline_regression.png
              ${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_custom_series_regression.png
              ${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_pictorial_bar_regression.png
              ${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_gauge_regression.png
              ${CMAKE_CURRENT_BINARY_DIR}/test_output/echarts_image_scatter_regression.png
    )
    set_tests_properties(echarts_regression_verify PROPERTIES
      DEPENDS echarts_regression_render
    )
  endif()
endif()
