#include "canvas_engine/canvas/ColorParser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <optional>
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

bool EndsWith(std::string_view text, std::string_view suffix) {
  return text.size() >= suffix.size() &&
         text.substr(text.size() - suffix.size()) == suffix;
}

std::vector<std::string> SplitSpaceSlashComponents(std::string_view value) {
  std::vector<std::string> parts;
  std::string current;
  for (char ch : value) {
    if (std::isspace(static_cast<unsigned char>(ch))) {
      if (!current.empty()) {
        parts.push_back(current);
        current.clear();
      }
      continue;
    }
    if (ch == '/') {
      if (!current.empty()) {
        parts.push_back(current);
        current.clear();
      }
      parts.emplace_back("/");
      continue;
    }
    current.push_back(ch);
  }
  if (!current.empty()) {
    parts.push_back(current);
  }
  return parts;
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

bool ParsePercentage(std::string_view text, float* result) {
  if (!EndsWith(text, "%")) {
    return false;
  }
  return ParseFloat(text.substr(0, text.size() - 1), result);
}

bool ParseRgbChannel(std::string_view text, uint8_t* result) {
  float value = 0.0f;
  if (ParsePercentage(text, &value)) {
    *result = ClampByte(static_cast<int>(std::lround(value * 255.0f / 100.0f)));
    return true;
  }
  if (!ParseFloat(text, &value)) {
    return false;
  }
  *result = ClampByte(static_cast<int>(std::lround(value)));
  return true;
}

bool ParseAlpha(std::string_view text, uint8_t* result) {
  float value = 0.0f;
  if (ParsePercentage(text, &value)) {
    *result = ClampAlpha(value / 100.0f);
    return true;
  }
  if (!ParseFloat(text, &value)) {
    return false;
  }
  *result = ClampAlpha(value);
  return true;
}

bool ParseHue(std::string_view text, float* result) {
  float value = 0.0f;
  if (EndsWith(text, "deg")) {
    if (!ParseFloat(text.substr(0, text.size() - 3), &value)) {
      return false;
    }
  } else if (EndsWith(text, "rad")) {
    if (!ParseFloat(text.substr(0, text.size() - 3), &value)) {
      return false;
    }
    constexpr float kPi = 3.14159265358979323846f;
    value = value * 180.0f / kPi;
  } else if (EndsWith(text, "turn")) {
    if (!ParseFloat(text.substr(0, text.size() - 4), &value)) {
      return false;
    }
    value *= 360.0f;
  } else if (EndsWith(text, "grad")) {
    if (!ParseFloat(text.substr(0, text.size() - 4), &value)) {
      return false;
    }
    value *= 0.9f;
  } else if (!ParseFloat(text, &value)) {
    return false;
  }

  value = std::fmod(value, 360.0f);
  if (value < 0.0f) {
    value += 360.0f;
  }
  *result = value;
  return true;
}

float HueToRgb(float p, float q, float t) {
  if (t < 0.0f) {
    t += 1.0f;
  }
  if (t > 1.0f) {
    t -= 1.0f;
  }
  if (t < 1.0f / 6.0f) {
    return p + (q - p) * 6.0f * t;
  }
  if (t < 1.0f / 2.0f) {
    return q;
  }
  if (t < 2.0f / 3.0f) {
    return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
  }
  return p;
}

SkColor HslToRgb(float hue, float saturation, float lightness, uint8_t alpha) {
  const float h = hue / 360.0f;
  const float s = std::clamp(saturation / 100.0f, 0.0f, 1.0f);
  const float l = std::clamp(lightness / 100.0f, 0.0f, 1.0f);

  float r = l;
  float g = l;
  float b = l;
  if (s != 0.0f) {
    const float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    const float p = 2.0f * l - q;
    r = HueToRgb(p, q, h + 1.0f / 3.0f);
    g = HueToRgb(p, q, h);
    b = HueToRgb(p, q, h - 1.0f / 3.0f);
  }

  return SkColorSetARGB(alpha, ClampByte(static_cast<int>(std::lround(r * 255.0f))),
                        ClampByte(static_cast<int>(std::lround(g * 255.0f))),
                        ClampByte(static_cast<int>(std::lround(b * 255.0f))));
}

std::optional<SkColor> ParseRgbFunction(std::string_view input, bool has_alpha) {
  const std::string prefix = has_alpha ? "rgba(" : "rgb(";
  if (input.rfind(prefix, 0) != 0 || input.back() != ')') {
    return std::nullopt;
  }

  const auto body = input.substr(prefix.size(), input.size() - prefix.size() - 1);
  std::vector<std::string> components;
  std::string alpha_component;
  if (body.find(',') != std::string_view::npos) {
    components = SplitComponents(body);
    if (components.size() == 4u) {
      alpha_component = components.back();
      components.pop_back();
    }
  } else {
    components = SplitSpaceSlashComponents(body);
    auto slash = std::find(components.begin(), components.end(), "/");
    if (slash != components.end()) {
      if (std::distance(slash, components.end()) != 2) {
        return std::nullopt;
      }
      alpha_component = *(slash + 1);
      components.erase(slash, components.end());
    }
  }

  if (components.size() != 3u || (has_alpha && alpha_component.empty())) {
    return std::nullopt;
  }

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  if (!ParseRgbChannel(components[0], &r) || !ParseRgbChannel(components[1], &g) ||
      !ParseRgbChannel(components[2], &b)) {
    return std::nullopt;
  }

  uint8_t alpha = 255;
  if (!alpha_component.empty()) {
    if (!ParseAlpha(alpha_component, &alpha)) {
      return std::nullopt;
    }
  }

  return SkColorSetARGB(alpha, r, g, b);
}

std::optional<SkColor> ParseHslFunction(std::string_view input, bool has_alpha) {
  const std::string prefix = has_alpha ? "hsla(" : "hsl(";
  if (input.rfind(prefix, 0) != 0 || input.back() != ')') {
    return std::nullopt;
  }

  const auto body = input.substr(prefix.size(), input.size() - prefix.size() - 1);
  std::vector<std::string> components;
  std::string alpha_component;
  if (body.find(',') != std::string_view::npos) {
    components = SplitComponents(body);
    if (components.size() == 4u) {
      alpha_component = components.back();
      components.pop_back();
    }
  } else {
    components = SplitSpaceSlashComponents(body);
    auto slash = std::find(components.begin(), components.end(), "/");
    if (slash != components.end()) {
      if (std::distance(slash, components.end()) != 2) {
        return std::nullopt;
      }
      alpha_component = *(slash + 1);
      components.erase(slash, components.end());
    }
  }

  if (components.size() != 3u || (has_alpha && alpha_component.empty())) {
    return std::nullopt;
  }

  float hue = 0.0f;
  float saturation = 0.0f;
  float lightness = 0.0f;
  if (!ParseHue(components[0], &hue) ||
      !ParsePercentage(components[1], &saturation) ||
      !ParsePercentage(components[2], &lightness)) {
    return std::nullopt;
  }

  uint8_t alpha = 255;
  if (!alpha_component.empty() && !ParseAlpha(alpha_component, &alpha)) {
    return std::nullopt;
  }

  return HslToRgb(hue, saturation, lightness, alpha);
}

const std::unordered_map<std::string, SkColor>& NamedColors() {
  static const auto* colors = new std::unordered_map<std::string, SkColor>({
      {"aliceblue", SkColorSetARGB(255, 240, 248, 255)},
      {"antiquewhite", SkColorSetARGB(255, 250, 235, 215)},
      {"aqua", SkColorSetARGB(255, 0, 255, 255)},
      {"aquamarine", SkColorSetARGB(255, 127, 255, 212)},
      {"azure", SkColorSetARGB(255, 240, 255, 255)},
      {"beige", SkColorSetARGB(255, 245, 245, 220)},
      {"bisque", SkColorSetARGB(255, 255, 228, 196)},
      {"black", SkColorSetARGB(255, 0, 0, 0)},
      {"blanchedalmond", SkColorSetARGB(255, 255, 235, 205)},
      {"blue", SkColorSetARGB(255, 0, 0, 255)},
      {"blueviolet", SkColorSetARGB(255, 138, 43, 226)},
      {"brown", SkColorSetARGB(255, 165, 42, 42)},
      {"burlywood", SkColorSetARGB(255, 222, 184, 135)},
      {"cadetblue", SkColorSetARGB(255, 95, 158, 160)},
      {"chartreuse", SkColorSetARGB(255, 127, 255, 0)},
      {"chocolate", SkColorSetARGB(255, 210, 105, 30)},
      {"coral", SkColorSetARGB(255, 255, 127, 80)},
      {"cornflowerblue", SkColorSetARGB(255, 100, 149, 237)},
      {"cornsilk", SkColorSetARGB(255, 255, 248, 220)},
      {"crimson", SkColorSetARGB(255, 220, 20, 60)},
      {"cyan", SkColorSetARGB(255, 0, 255, 255)},
      {"darkblue", SkColorSetARGB(255, 0, 0, 139)},
      {"darkcyan", SkColorSetARGB(255, 0, 139, 139)},
      {"darkgoldenrod", SkColorSetARGB(255, 184, 134, 11)},
      {"darkgray", SkColorSetARGB(255, 169, 169, 169)},
      {"darkgreen", SkColorSetARGB(255, 0, 100, 0)},
      {"darkgrey", SkColorSetARGB(255, 169, 169, 169)},
      {"darkkhaki", SkColorSetARGB(255, 189, 183, 107)},
      {"darkmagenta", SkColorSetARGB(255, 139, 0, 139)},
      {"darkolivegreen", SkColorSetARGB(255, 85, 107, 47)},
      {"darkorange", SkColorSetARGB(255, 255, 140, 0)},
      {"darkorchid", SkColorSetARGB(255, 153, 50, 204)},
      {"darkred", SkColorSetARGB(255, 139, 0, 0)},
      {"darksalmon", SkColorSetARGB(255, 233, 150, 122)},
      {"darkseagreen", SkColorSetARGB(255, 143, 188, 143)},
      {"darkslateblue", SkColorSetARGB(255, 72, 61, 139)},
      {"darkslategray", SkColorSetARGB(255, 47, 79, 79)},
      {"darkslategrey", SkColorSetARGB(255, 47, 79, 79)},
      {"darkturquoise", SkColorSetARGB(255, 0, 206, 209)},
      {"darkviolet", SkColorSetARGB(255, 148, 0, 211)},
      {"deeppink", SkColorSetARGB(255, 255, 20, 147)},
      {"deepskyblue", SkColorSetARGB(255, 0, 191, 255)},
      {"dimgray", SkColorSetARGB(255, 105, 105, 105)},
      {"dimgrey", SkColorSetARGB(255, 105, 105, 105)},
      {"dodgerblue", SkColorSetARGB(255, 30, 144, 255)},
      {"firebrick", SkColorSetARGB(255, 178, 34, 34)},
      {"floralwhite", SkColorSetARGB(255, 255, 250, 240)},
      {"forestgreen", SkColorSetARGB(255, 34, 139, 34)},
      {"fuchsia", SkColorSetARGB(255, 255, 0, 255)},
      {"gainsboro", SkColorSetARGB(255, 220, 220, 220)},
      {"ghostwhite", SkColorSetARGB(255, 248, 248, 255)},
      {"gold", SkColorSetARGB(255, 255, 215, 0)},
      {"goldenrod", SkColorSetARGB(255, 218, 165, 32)},
      {"gray", SkColorSetARGB(255, 128, 128, 128)},
      {"green", SkColorSetARGB(255, 0, 128, 0)},
      {"greenyellow", SkColorSetARGB(255, 173, 255, 47)},
      {"grey", SkColorSetARGB(255, 128, 128, 128)},
      {"honeydew", SkColorSetARGB(255, 240, 255, 240)},
      {"hotpink", SkColorSetARGB(255, 255, 105, 180)},
      {"indianred", SkColorSetARGB(255, 205, 92, 92)},
      {"indigo", SkColorSetARGB(255, 75, 0, 130)},
      {"ivory", SkColorSetARGB(255, 255, 255, 240)},
      {"khaki", SkColorSetARGB(255, 240, 230, 140)},
      {"lavender", SkColorSetARGB(255, 230, 230, 250)},
      {"lavenderblush", SkColorSetARGB(255, 255, 240, 245)},
      {"lawngreen", SkColorSetARGB(255, 124, 252, 0)},
      {"lemonchiffon", SkColorSetARGB(255, 255, 250, 205)},
      {"lightblue", SkColorSetARGB(255, 173, 216, 230)},
      {"lightcoral", SkColorSetARGB(255, 240, 128, 128)},
      {"lightcyan", SkColorSetARGB(255, 224, 255, 255)},
      {"lightgoldenrodyellow", SkColorSetARGB(255, 250, 250, 210)},
      {"lightgray", SkColorSetARGB(255, 211, 211, 211)},
      {"lightgreen", SkColorSetARGB(255, 144, 238, 144)},
      {"lightgrey", SkColorSetARGB(255, 211, 211, 211)},
      {"lightpink", SkColorSetARGB(255, 255, 182, 193)},
      {"lightsalmon", SkColorSetARGB(255, 255, 160, 122)},
      {"lightseagreen", SkColorSetARGB(255, 32, 178, 170)},
      {"lightskyblue", SkColorSetARGB(255, 135, 206, 250)},
      {"lightslategray", SkColorSetARGB(255, 119, 136, 153)},
      {"lightslategrey", SkColorSetARGB(255, 119, 136, 153)},
      {"lightsteelblue", SkColorSetARGB(255, 176, 196, 222)},
      {"lightyellow", SkColorSetARGB(255, 255, 255, 224)},
      {"lime", SkColorSetARGB(255, 0, 255, 0)},
      {"limegreen", SkColorSetARGB(255, 50, 205, 50)},
      {"linen", SkColorSetARGB(255, 250, 240, 230)},
      {"magenta", SkColorSetARGB(255, 255, 0, 255)},
      {"maroon", SkColorSetARGB(255, 128, 0, 0)},
      {"mediumaquamarine", SkColorSetARGB(255, 102, 205, 170)},
      {"mediumblue", SkColorSetARGB(255, 0, 0, 205)},
      {"mediumorchid", SkColorSetARGB(255, 186, 85, 211)},
      {"mediumpurple", SkColorSetARGB(255, 147, 112, 219)},
      {"mediumseagreen", SkColorSetARGB(255, 60, 179, 113)},
      {"mediumslateblue", SkColorSetARGB(255, 123, 104, 238)},
      {"mediumspringgreen", SkColorSetARGB(255, 0, 250, 154)},
      {"mediumturquoise", SkColorSetARGB(255, 72, 209, 204)},
      {"mediumvioletred", SkColorSetARGB(255, 199, 21, 133)},
      {"midnightblue", SkColorSetARGB(255, 25, 25, 112)},
      {"mintcream", SkColorSetARGB(255, 245, 255, 250)},
      {"mistyrose", SkColorSetARGB(255, 255, 228, 225)},
      {"moccasin", SkColorSetARGB(255, 255, 228, 181)},
      {"navajowhite", SkColorSetARGB(255, 255, 222, 173)},
      {"navy", SkColorSetARGB(255, 0, 0, 128)},
      {"oldlace", SkColorSetARGB(255, 253, 245, 230)},
      {"olive", SkColorSetARGB(255, 128, 128, 0)},
      {"olivedrab", SkColorSetARGB(255, 107, 142, 35)},
      {"orange", SkColorSetARGB(255, 255, 165, 0)},
      {"orangered", SkColorSetARGB(255, 255, 69, 0)},
      {"orchid", SkColorSetARGB(255, 218, 112, 214)},
      {"palegoldenrod", SkColorSetARGB(255, 238, 232, 170)},
      {"palegreen", SkColorSetARGB(255, 152, 251, 152)},
      {"paleturquoise", SkColorSetARGB(255, 175, 238, 238)},
      {"palevioletred", SkColorSetARGB(255, 219, 112, 147)},
      {"papayawhip", SkColorSetARGB(255, 255, 239, 213)},
      {"peachpuff", SkColorSetARGB(255, 255, 218, 185)},
      {"peru", SkColorSetARGB(255, 205, 133, 63)},
      {"pink", SkColorSetARGB(255, 255, 192, 203)},
      {"plum", SkColorSetARGB(255, 221, 160, 221)},
      {"powderblue", SkColorSetARGB(255, 176, 224, 230)},
      {"purple", SkColorSetARGB(255, 128, 0, 128)},
      {"rebeccapurple", SkColorSetARGB(255, 102, 51, 153)},
      {"red", SkColorSetARGB(255, 255, 0, 0)},
      {"rosybrown", SkColorSetARGB(255, 188, 143, 143)},
      {"royalblue", SkColorSetARGB(255, 65, 105, 225)},
      {"saddlebrown", SkColorSetARGB(255, 139, 69, 19)},
      {"salmon", SkColorSetARGB(255, 250, 128, 114)},
      {"sandybrown", SkColorSetARGB(255, 244, 164, 96)},
      {"seagreen", SkColorSetARGB(255, 46, 139, 87)},
      {"seashell", SkColorSetARGB(255, 255, 245, 238)},
      {"sienna", SkColorSetARGB(255, 160, 82, 45)},
      {"silver", SkColorSetARGB(255, 192, 192, 192)},
      {"skyblue", SkColorSetARGB(255, 135, 206, 235)},
      {"slateblue", SkColorSetARGB(255, 106, 90, 205)},
      {"slategray", SkColorSetARGB(255, 112, 128, 144)},
      {"slategrey", SkColorSetARGB(255, 112, 128, 144)},
      {"snow", SkColorSetARGB(255, 255, 250, 250)},
      {"springgreen", SkColorSetARGB(255, 0, 255, 127)},
      {"steelblue", SkColorSetARGB(255, 70, 130, 180)},
      {"tan", SkColorSetARGB(255, 210, 180, 140)},
      {"teal", SkColorSetARGB(255, 0, 128, 128)},
      {"thistle", SkColorSetARGB(255, 216, 191, 216)},
      {"tomato", SkColorSetARGB(255, 255, 99, 71)},
      {"transparent", SkColorSetARGB(0, 0, 0, 0)},
      {"turquoise", SkColorSetARGB(255, 64, 224, 208)},
      {"violet", SkColorSetARGB(255, 238, 130, 238)},
      {"wheat", SkColorSetARGB(255, 245, 222, 179)},
      {"white", SkColorSetARGB(255, 255, 255, 255)},
      {"whitesmoke", SkColorSetARGB(255, 245, 245, 245)},
      {"yellow", SkColorSetARGB(255, 255, 255, 0)},
      {"yellowgreen", SkColorSetARGB(255, 154, 205, 50)},
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
  if (auto hsl = ParseHslFunction(normalized, false); hsl.has_value()) {
    return hsl;
  }
  if (auto hsla = ParseHslFunction(normalized, true); hsla.has_value()) {
    return hsla;
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
