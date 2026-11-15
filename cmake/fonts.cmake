option(CANVAS_ENGINE_BUNDLED_FONTS "Bundle pinned Noto fonts" ON)
set(CANVAS_ENGINE_FONT_CACHE "" CACHE PATH
    "Directory used to cache pinned font archives")

if(CANVAS_ENGINE_BUNDLED_FONTS)
  if(NOT CANVAS_ENGINE_FONT_CACHE AND
     DEFINED ENV{CANVAS_ENGINE_FONT_CACHE} AND
     NOT "$ENV{CANVAS_ENGINE_FONT_CACHE}" STREQUAL "")
    get_filename_component(CANVAS_ENGINE_FONT_CACHE
      "$ENV{CANVAS_ENGINE_FONT_CACHE}" ABSOLUTE)
  elseif(NOT CANVAS_ENGINE_FONT_CACHE)
    set(CANVAS_ENGINE_FONT_CACHE "${CMAKE_BINARY_DIR}/font-cache")
  endif()

  set(CANVAS_ENGINE_FONTS_DIR "${CMAKE_BINARY_DIR}/fonts")
  file(MAKE_DIRECTORY "${CANVAS_ENGINE_FONT_CACHE}"
                      "${CANVAS_ENGINE_FONTS_DIR}")

  function(canvas_engine_fetch_font_archive filename url sha256)
    set(archive "${CANVAS_ENGINE_FONT_CACHE}/${filename}")
    if(EXISTS "${archive}")
      file(SHA256 "${archive}" actual_sha256)
      if(NOT actual_sha256 STREQUAL sha256)
        file(REMOVE "${archive}")
        file(REMOVE_RECURSE "${archive}.extracted")
      endif()
    endif()
    if(NOT EXISTS "${archive}")
      file(DOWNLOAD "${url}" "${archive}"
        EXPECTED_HASH "SHA256=${sha256}"
        STATUS download_status
        SHOW_PROGRESS
      )
      list(GET download_status 0 download_code)
      list(GET download_status 1 download_message)
      if(NOT download_code EQUAL 0)
        file(REMOVE "${archive}")
        message(FATAL_ERROR
          "Failed to download or verify ${filename}: ${download_message}")
      endif()
    endif()
    file(SHA256 "${archive}" actual_sha256)
    if(NOT actual_sha256 STREQUAL sha256)
      file(REMOVE "${archive}")
      file(REMOVE_RECURSE "${archive}.extracted")
      message(FATAL_ERROR
        "SHA-256 mismatch for ${filename}: expected ${sha256}, got ${actual_sha256}")
    endif()
    set(${filename}_ARCHIVE "${archive}" PARENT_SCOPE)
  endfunction()

  canvas_engine_fetch_font_archive(
    "NotoSans-v2.015.zip"
    "https://github.com/notofonts/latin-greek-cyrillic/releases/download/NotoSans-v2.015/NotoSans-v2.015.zip"
    "0c34df072a3fa7efbb7cbf34950e1f971a4447cffe365d3a359e2d4089b958f5")
  canvas_engine_fetch_font_archive(
    "NotoSerif-v2.015.zip"
    "https://github.com/notofonts/latin-greek-cyrillic/releases/download/NotoSerif-v2.015/NotoSerif-v2.015.zip"
    "0e9a43c8a4b94ac76f55069ed1d7385bbcaf6b99527a94deb5619e032b7e76c1")
  canvas_engine_fetch_font_archive(
    "NotoSansMono-v2.014.zip"
    "https://github.com/notofonts/latin-greek-cyrillic/releases/download/NotoSansMono-v2.014/NotoSansMono-v2.014.zip"
    "090cf6c5e03f337a755630ca888b1fef463e64ae7b33ee134e9309c05f978732")
  canvas_engine_fetch_font_archive(
    "18_NotoSansSC.zip"
    "https://github.com/notofonts/noto-cjk/releases/download/Sans2.004/18_NotoSansSC.zip"
    "4d107c09ada479d3e48b6e78c83835773cbd9214bf6e12cdb7b60f8e068292ec")

  foreach(archive_name IN ITEMS
      "NotoSans-v2.015.zip" "NotoSerif-v2.015.zip"
      "NotoSansMono-v2.014.zip" "18_NotoSansSC.zip")
    set(extract_dir "${CANVAS_ENGINE_FONT_CACHE}/${archive_name}.extracted")
    if(NOT EXISTS "${extract_dir}/.complete")
      file(REMOVE_RECURSE "${extract_dir}")
      file(MAKE_DIRECTORY "${extract_dir}")
      file(ARCHIVE_EXTRACT INPUT "${CANVAS_ENGINE_FONT_CACHE}/${archive_name}"
        DESTINATION "${extract_dir}")
      file(WRITE "${extract_dir}/.complete" "")
    endif()
  endforeach()

  set(font_sources
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSans-v2.015.zip.extracted/NotoSans/hinted/ttf/NotoSans-Regular.ttf"
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSans-v2.015.zip.extracted/NotoSans/hinted/ttf/NotoSans-Italic.ttf"
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSans-v2.015.zip.extracted/NotoSans/hinted/ttf/NotoSans-Medium.ttf"
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSans-v2.015.zip.extracted/NotoSans/hinted/ttf/NotoSans-Bold.ttf"
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSans-v2.015.zip.extracted/NotoSans/hinted/ttf/NotoSans-BoldItalic.ttf"
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSerif-v2.015.zip.extracted/NotoSerif/hinted/ttf/NotoSerif-Regular.ttf"
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSerif-v2.015.zip.extracted/NotoSerif/hinted/ttf/NotoSerif-Bold.ttf"
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSansMono-v2.014.zip.extracted/NotoSansMono/hinted/ttf/NotoSansMono-Regular.ttf"
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSansMono-v2.014.zip.extracted/NotoSansMono/hinted/ttf/NotoSansMono-Bold.ttf"
    "${CANVAS_ENGINE_FONT_CACHE}/18_NotoSansSC.zip.extracted/NotoSansSC-Regular.otf"
    "${CANVAS_ENGINE_FONT_CACHE}/18_NotoSansSC.zip.extracted/NotoSansSC-Bold.otf"
  )
  set(font_names
    NotoSans-Regular.ttf NotoSans-Italic.ttf NotoSans-Medium.ttf
    NotoSans-Bold.ttf NotoSans-BoldItalic.ttf NotoSerif-Regular.ttf
    NotoSerif-Bold.ttf NotoSansMono-Regular.ttf NotoSansMono-Bold.ttf
    NotoSansSC-Regular.otf NotoSansSC-Bold.otf
  )
  list(LENGTH font_sources font_count)
  math(EXPR last_font "${font_count} - 1")
  foreach(index RANGE ${last_font})
    list(GET font_sources ${index} font_source)
    list(GET font_names ${index} font_name)
    if(NOT EXISTS "${font_source}")
      message(FATAL_ERROR "Pinned font archive is missing ${font_source}")
    endif()
    configure_file("${font_source}" "${CANVAS_ENGINE_FONTS_DIR}/${font_name}"
      COPYONLY)
  endforeach()
  configure_file(
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSans-v2.015.zip.extracted/OFL.txt"
    "${CANVAS_ENGINE_FONTS_DIR}/OFL.txt" COPYONLY)
  configure_file(
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSerif-v2.015.zip.extracted/OFL.txt"
    "${CANVAS_ENGINE_FONTS_DIR}/OFL-NotoSerif.txt" COPYONLY)
  configure_file(
    "${CANVAS_ENGINE_FONT_CACHE}/NotoSansMono-v2.014.zip.extracted/OFL.txt"
    "${CANVAS_ENGINE_FONTS_DIR}/OFL-NotoSansMono.txt" COPYONLY)
  configure_file(
    "${CANVAS_ENGINE_FONT_CACHE}/18_NotoSansSC.zip.extracted/LICENSE"
    "${CANVAS_ENGINE_FONTS_DIR}/LICENSE-NotoSansSC.txt" COPYONLY)
else()
  message(WARNING
    "Bundled fonts are disabled; text output depends on installed system fonts.")
  set(CANVAS_ENGINE_FONTS_DIR "")
endif()
