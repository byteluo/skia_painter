#include "canvas_engine/canvas/ColorParser.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "include/core/SkTypes.h"

namespace canvas_engine {

namespace {

std::string Trim(std::string_view text) {
  size_t begin = 0;
  size_t end = text.size();

  while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }

  return std::string(text.substr(begin, end - begin));
}

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

int HexValue(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return 10 + (ch - 'a');
  }
  if (ch >= 'A' && ch <= 'F') {
    return 10 + (ch - 'A');
  }
  return -1;
}

std::optional<uint8_t> ParseHexByte(char high, char low) {
  int high_value = HexValue(high);
  int low_value = HexValue(low);
  if (high_value < 0 || low_value < 0) {
    return std::nullopt;
  }
  return static_cast<uint8_t>((high_value << 4) | low_value);
}

std::optional<SkColor> ParseHexColor(std::string_view value) {
  if (value.empty() || value.front() != '#') {
    return std::nullopt;
  }

  if (value.size() == 4 || value.size() == 5) {
    const int r = HexValue(value[1]);
    const int g = HexValue(value[2]);
    const int b = HexValue(value[3]);
    const int a = value.size() == 5 ? HexValue(value[4]) : 15;
    if (r < 0 || g < 0 || b < 0 || a < 0) {
      return std::nullopt;
    }
    return SkColorSetARGB(static_cast<U8CPU>(a * 17), static_cast<U8CPU>(r * 17),
                          static_cast<U8CPU>(g * 17), static_cast<U8CPU>(b * 17));
  }

  if (value.size() == 7 || value.size() == 9) {
    auto r = ParseHexByte(value[1], value[2]);
    auto g = ParseHexByte(value[3], value[4]);
    auto b = ParseHexByte(value[5], value[6]);
    auto a = value.size() == 9 ? ParseHexByte(value[7], value[8])
                               : std::optional<uint8_t>(255);
    if (!r.has_value() || !g.has_value() || !b.has_value() || !a.has_value()) {
      return std::nullopt;
    }
    return SkColorSetARGB(a.value(), r.value(), g.value(), b.value());
  }

  return std::nullopt;
}

std::vector<std::string> SplitComponents(std::string_view value) {
  std::vector<std::string> parts;
  size_t start = 0;
  while (start < value.size()) {
    size_t comma = value.find(',', start);
    if (comma == std::string_view::npos) {
      parts.push_back(Trim(value.substr(start)));
      break;
    }
    parts.push_back(Trim(value.substr(start, comma - start)));
    start = comma + 1;
  }
  return parts;
}

bool ParseInteger(std::string_view text, int* result) {
  int value = 0;
  auto first = text.data();
  auto last = text.data() + text.size();
  auto parsed = std::from_chars(first, last, value);
  if (parsed.ec != std::errc() || parsed.ptr != last) {
    return false;
  }
  *result = value;
  return true;
}

bool ParseFloat(std::string_view text, float* result) {
  std::string normalized(text);
  char* end = nullptr;
  const float value = std::strtof(normalized.c_str(), &end);
  if (end == normalized.c_str() || *end != '\0') {
    return false;
  }
  *result = value;
  return true;
}

uint8_t ClampByte(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > 255) {
    return 255;
  }
  return static_cast<uint8_t>(value);
}

uint8_t ClampAlpha(float value) {
  float clamped = std::clamp(value, 0.0f, 1.0f);
  return static_cast<uint8_t>(std::lround(clamped * 255.0f));
}

std::optional<SkColor> ParseRgbFunction(std::string_view input, bool has_alpha) {
  const std::string prefix = has_alpha ? "rgba(" : "rgb(";
  if (input.rfind(prefix, 0) != 0 || input.back() != ')') {
    return std::nullopt;
  }

  auto body = input.substr(prefix.size(), input.size() - prefix.size() - 1);
  auto components = SplitComponents(body);
  if (components.size() != (has_alpha ? 4u : 3u)) {
    return std::nullopt;
  }

  int r = 0;
  int g = 0;
  int b = 0;
  if (!ParseInteger(components[0], &r) || !ParseInteger(components[1], &g) ||
      !ParseInteger(components[2], &b)) {
    return std::nullopt;
  }

  uint8_t alpha = 255;
  if (has_alpha) {
    float a = 0.0f;
    if (!ParseFloat(components[3], &a)) {
      return std::nullopt;
    }
    alpha = ClampAlpha(a);
  }

  return SkColorSetARGB(alpha, ClampByte(r), ClampByte(g), ClampByte(b));
}

const std::unordered_map<std::string, SkColor>& NamedColors() {
  static const auto* colors = new std::unordered_map<std::string, SkColor>({
      {"black", SK_ColorBLACK},
      {"white", SK_ColorWHITE},
      {"red", SK_ColorRED},
      {"green", SkColorSetARGB(255, 0, 128, 0)},
      {"blue", SK_ColorBLUE},
      {"yellow", SK_ColorYELLOW},
      {"gray", SkColorSetARGB(255, 128, 128, 128)},
      {"grey", SkColorSetARGB(255, 128, 128, 128)},
      {"orange", SkColorSetARGB(255, 255, 165, 0)},
      {"purple", SkColorSetARGB(255, 128, 0, 128)},
      {"transparent", SkColorSetARGB(0, 0, 0, 0)},
  });
  return *colors;
}

}  // namespace

std::optional<SkColor> ParseCssColor(std::string_view css_color) {
  std::string normalized = ToLower(Trim(css_color));
  if (normalized.empty()) {
    return std::nullopt;
  }

  if (auto hex = ParseHexColor(normalized); hex.has_value()) {
    return hex;
  }
  if (auto rgb = ParseRgbFunction(normalized, false); rgb.has_value()) {
    return rgb;
  }
  if (auto rgba = ParseRgbFunction(normalized, true); rgba.has_value()) {
    return rgba;
  }

  const auto& colors = NamedColors();
  auto iterator = colors.find(normalized);
  if (iterator != colors.end()) {
    return iterator->second;
  }

  return std::nullopt;
}

std::string NormalizeCssColor(std::string_view css_color) {
  return ToLower(Trim(css_color));
}

}  // namespace canvas_engine
