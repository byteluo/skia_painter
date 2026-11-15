#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkTypeface.h"

namespace canvas_engine {

class FontManager {
 public:
  FontManager(std::filesystem::path font_dir, bool allow_system_fonts);

  bool valid() const { return bundled_ || system_; }
  bool has_bundled_fonts() const { return static_cast<bool>(bundled_); }
  bool allows_system_fonts() const { return allow_system_fonts_; }

  sk_sp<SkTypeface> Match(std::string_view family,
                          const SkFontStyle& style) const;
  sk_sp<SkTypeface> MatchGlyph(
      const std::vector<std::string>& families, const SkFontStyle& style,
      SkUnichar codepoint) const;

 private:
  sk_sp<SkFontMgr> bundled_;
  sk_sp<SkFontMgr> system_;
  bool allow_system_fonts_ = false;
};

}  // namespace canvas_engine
