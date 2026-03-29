#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "include/core/SkColor.h"

namespace canvas_engine {

std::optional<SkColor> ParseCssColor(std::string_view css_color);
std::string NormalizeCssColor(std::string_view css_color);

}  // namespace canvas_engine
