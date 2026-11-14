#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "include/core/SkFontStyle.h"

namespace canvas_engine {

struct FontDescriptor {
  float size_px = 10.0f;
  int weight = 400;
  SkFontStyle::Slant slant = SkFontStyle::kUpright_Slant;
  std::vector<std::string> families = {"sans-serif"};
};

std::optional<FontDescriptor> ParseCssFont(std::string_view css);

}  // namespace canvas_engine
