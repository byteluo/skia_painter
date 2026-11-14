#include <cmath>
#include <iostream>
#include <string_view>
#include <vector>

#include "canvas_engine/canvas/FontParser.h"

namespace {

bool ExpectFont(std::string_view css, float size_px, int weight,
                SkFontStyle::Slant slant,
                const std::vector<std::string>& families) {
  const auto parsed = canvas_engine::ParseCssFont(css);
  if (!parsed.has_value()) {
    std::cerr << "ParseCssFont returned nullopt for " << css << '\n';
    return false;
  }
  const auto& font = parsed.value();
  if (std::abs(font.size_px - size_px) > 0.01f || font.weight != weight ||
      font.slant != slant || font.families != families) {
    std::cerr << "unexpected descriptor for " << css << '\n';
    return false;
  }
  return true;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= ExpectFont("bold 12px sans-serif", 12.0f, 700,
                   SkFontStyle::kUpright_Slant, {"sans-serif"});
  ok &= ExpectFont("italic 600 14px/1.5 'Microsoft YaHei', Arial, sans-serif",
                   14.0f, 600, SkFontStyle::kItalic_Slant,
                   {"Microsoft YaHei", "Arial", "sans-serif"});
  ok &= ExpectFont("normal normal 12px Arial", 12.0f, 400,
                   SkFontStyle::kUpright_Slant, {"Arial"});
  ok &= ExpectFont("12pt serif", 16.0f, 400, SkFontStyle::kUpright_Slant,
                   {"serif"});
  ok &= ExpectFont("bolder 10px a", 10.0f, 700,
                   SkFontStyle::kUpright_Slant, {"a"});
  ok &= ExpectFont("oblique 300 small-caps 20px x", 20.0f, 300,
                   SkFontStyle::kOblique_Slant, {"x"});
  ok &= ExpectFont("16px Arial  Bold", 16.0f, 400,
                   SkFontStyle::kUpright_Slant, {"Arial Bold"});
  ok &= ExpectFont("1em monospace", 16.0f, 400,
                   SkFontStyle::kUpright_Slant, {"monospace"});

  for (const auto invalid : {"", "bold", "12px", "px Arial"}) {
    if (canvas_engine::ParseCssFont(invalid).has_value()) {
      std::cerr << "invalid font unexpectedly parsed: " << invalid << '\n';
      ok = false;
    }
  }
  return ok ? 0 : 1;
}
