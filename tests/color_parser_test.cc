#include <cmath>
#include <iostream>
#include <string_view>

#include "canvas_engine/canvas/ColorParser.h"
#include "include/core/SkColor.h"

namespace {

bool ExpectColor(std::string_view css, int r, int g, int b, int a = 255) {
  const auto parsed = canvas_engine::ParseCssColor(css);
  if (!parsed.has_value()) {
    std::cerr << "ParseCssColor returned nullopt for " << css << '\n';
    return false;
  }

  const SkColor color = parsed.value();
  const bool ok = std::abs(static_cast<int>(SkColorGetR(color)) - r) <= 1 &&
                  std::abs(static_cast<int>(SkColorGetG(color)) - g) <= 1 &&
                  std::abs(static_cast<int>(SkColorGetB(color)) - b) <= 1 &&
                  std::abs(static_cast<int>(SkColorGetA(color)) - a) <= 1;
  if (!ok) {
    std::cerr << css << " parsed as rgba(" << static_cast<int>(SkColorGetR(color))
              << ", " << static_cast<int>(SkColorGetG(color)) << ", "
              << static_cast<int>(SkColorGetB(color)) << ", "
              << static_cast<int>(SkColorGetA(color)) << ")\n";
  }
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= ExpectColor("gold", 255, 215, 0);
  ok &= ExpectColor("lightblue", 173, 216, 230);
  ok &= ExpectColor("hsl(120, 50%, 40%)", 51, 153, 51);
  ok &= ExpectColor("hsla(0, 100%, 50%, 0.3)", 255, 0, 0, 77);
  ok &= ExpectColor("rgb(50%, 25%, 100%)", 128, 64, 255);
  ok &= ExpectColor("rgb(255 0 0 / 0.5)", 255, 0, 0, 128);

  if (canvas_engine::ParseCssColor("not-a-color").has_value()) {
    std::cerr << "invalid color unexpectedly parsed\n";
    ok = false;
  }

  return ok ? 0 : 1;
}
