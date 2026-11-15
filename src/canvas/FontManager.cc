#include "canvas_engine/canvas/FontManager.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>

#if defined(__linux__)
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_FreeType.h"
#endif

#include "include/ports/SkFontMgr_directory.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkTypeface.h"

namespace canvas_engine {
namespace {

std::string Lower(std::string_view value) {
  std::string lower(value);
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return lower;
}

std::string BundledFamily(std::string_view family) {
  const std::string lower = Lower(family);
  if (lower == "sans-serif" || lower == "system-ui" ||
      lower == "-apple-system" || lower == "helvetica" || lower == "arial" ||
      lower == "pingfang sc" || lower == "microsoft yahei") {
    return "Noto Sans";
  }
  if (lower == "serif" || lower == "times new roman") {
    return "Noto Serif";
  }
  if (lower == "monospace" || lower == "menlo" || lower == "courier") {
    return "Noto Sans Mono";
  }
  return std::string(family);
}

}  // namespace

FontManager::FontManager(std::filesystem::path font_dir,
                         bool allow_system_fonts)
    : allow_system_fonts_(allow_system_fonts) {
#if defined(__APPLE__)
  if (allow_system_fonts_) {
    static std::once_flag warning_once;
    std::call_once(warning_once, [] {
      std::cerr << "Warning: system-font fallback is unsupported on macOS in "
                   "this build; using bundled fonts only.\n";
    });
    allow_system_fonts_ = false;
  }
#endif
  if (!font_dir.empty()) {
    if (!std::filesystem::is_directory(font_dir)) {
      if (!allow_system_fonts_) {
        throw std::invalid_argument("font directory does not exist: " +
                                    font_dir.string());
      }
    } else {
      bundled_ = SkFontMgr_New_Custom_Directory(font_dir.string().c_str());
      if (!bundled_ && !allow_system_fonts_) {
        throw std::invalid_argument("failed to load fonts from directory: " +
                                    font_dir.string());
      }
    }
  }
  if (allow_system_fonts_) {
#if defined(__linux__)
    system_ = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#endif
  }
  if (!bundled_ && !system_) {
    throw std::invalid_argument(
        "no bundled fonts are available; provide --font-dir or enable "
        "--system-fonts");
  }
  if (bundled_ && !allow_system_fonts_) {
    auto style_set = bundled_->matchFamily("Noto Sans");
    if (style_set == nullptr || style_set->count() == 0) {
      throw std::invalid_argument(
          "font directory does not contain the required Noto Sans fallback: " +
          font_dir.string());
    }
  }
}

sk_sp<SkTypeface> FontManager::Match(std::string_view family,
                                     const SkFontStyle& style) const {
  const std::string mapped = BundledFamily(family);
  if (bundled_) {
    if (auto style_set = bundled_->matchFamily(mapped.c_str());
        style_set != nullptr && style_set->count() > 0) {
      return style_set->matchStyle(style);
    }
  }
  if (system_ && allow_system_fonts_) {
    const std::string requested(family);
    if (auto style_set = system_->matchFamily(requested.c_str());
        style_set != nullptr && style_set->count() > 0) {
      return style_set->matchStyle(style);
    }
    if (requested == "Noto Sans") {
      return system_->matchFamilyStyle(nullptr, style);
    }
  }
  return nullptr;
}

sk_sp<SkTypeface> FontManager::MatchGlyph(
    const std::vector<std::string>& families, const SkFontStyle& style,
    SkUnichar codepoint) const {
  const auto match_bundled = [this, &style](std::string_view family) {
    if (!bundled_) {
      return sk_sp<SkTypeface>();
    }
    const std::string mapped = BundledFamily(family);
    auto style_set = bundled_->matchFamily(mapped.c_str());
    return style_set != nullptr && style_set->count() > 0
               ? style_set->matchStyle(style)
               : sk_sp<SkTypeface>();
  };

  for (const auto& family : families) {
    auto typeface = match_bundled(family);
    if (typeface && typeface->unicharToGlyph(codepoint) != 0) {
      return typeface;
    }
  }
  auto typeface = match_bundled("Noto Sans SC");
  if (typeface && typeface->unicharToGlyph(codepoint) != 0) {
    return typeface;
  }
  if (system_ && allow_system_fonts_) {
    for (const auto& family : families) {
      const std::string requested(family);
      auto style_set = system_->matchFamily(requested.c_str());
      auto typeface = style_set != nullptr && style_set->count() > 0
                          ? style_set->matchStyle(style)
                          : nullptr;
      if (typeface && typeface->unicharToGlyph(codepoint) != 0) {
        return typeface;
      }
    }
    typeface = system_->matchFamilyStyle(nullptr, style);
    if (typeface && typeface->unicharToGlyph(codepoint) != 0) {
      return typeface;
    }
  }
  typeface = match_bundled("Noto Sans");
  if (typeface) {
    return typeface;
  }
  if (system_ && allow_system_fonts_) {
    return system_->matchFamilyStyle(nullptr, style);
  }
  return nullptr;
}

}  // namespace canvas_engine
