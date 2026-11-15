#include "canvas_engine/canvas/FontParser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace canvas_engine {

namespace {

struct Token {
  std::string_view value;
  std::size_t end = 0;
};

std::string_view Trim(std::string_view value) {
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.front()))) {
    value.remove_prefix(1);
  }
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.back()))) {
    value.remove_suffix(1);
  }
  return value;
}

std::string ToLower(std::string_view value) {
  std::string lower(value);
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char ch) {
                   return static_cast<char>(std::tolower(ch));
                 });
  return lower;
}

std::vector<Token> Tokenize(std::string_view css) {
  std::vector<Token> tokens;
  std::size_t position = 0;
  while (position < css.size()) {
    while (position < css.size() &&
           std::isspace(static_cast<unsigned char>(css[position]))) {
      ++position;
    }
    if (position == css.size()) {
      break;
    }
    const std::size_t start = position;
    while (position < css.size() &&
           !std::isspace(static_cast<unsigned char>(css[position]))) {
      ++position;
    }
    tokens.push_back({css.substr(start, position - start), position});
  }
  return tokens;
}

bool ParseSizeToken(std::string_view token, float* size_px) {
  const std::size_t slash = token.find('/');
  const std::string size_and_unit(token.substr(0, slash));
  char* number_end = nullptr;
  const float value = std::strtof(size_and_unit.c_str(), &number_end);
  if (number_end == size_and_unit.c_str() || !std::isfinite(value) ||
      !(value > 0.0f)) {
    return false;
  }

  const std::string unit = ToLower(
      std::string_view(number_end, size_and_unit.c_str() +
                                      size_and_unit.size() - number_end));
  if (unit == "px") {
    *size_px = value;
  } else if (unit == "pt") {
    *size_px = value * (4.0f / 3.0f);
  } else if (unit == "em" || unit == "rem") {
    *size_px = value * 16.0f;
  } else if (unit == "%") {
    *size_px = value * 0.16f;
  } else {
    return false;
  }
  return std::isfinite(*size_px) && *size_px > 0.0f;
}

bool IsStretchKeyword(std::string_view token) {
  const std::string lower = ToLower(token);
  return lower == "ultra-condensed" || lower == "extra-condensed" ||
         lower == "condensed" || lower == "semi-condensed" ||
         lower == "semi-expanded" || lower == "expanded" ||
         lower == "extra-expanded" || lower == "ultra-expanded";
}

bool ParseWeight(std::string_view token, int* weight) {
  const std::string lower = ToLower(token);
  if (lower == "bold" || lower == "bolder") {
    *weight = 700;
    return true;
  }
  if (lower == "lighter") {
    *weight = 300;
    return true;
  }

  if (lower.empty() ||
      !std::all_of(lower.begin(), lower.end(), [](unsigned char ch) {
        return std::isdigit(ch);
      })) {
    return false;
  }

  try {
    const int value = std::stoi(lower);
    if (value < 1 || value > 1000) {
      return false;
    }
    *weight = std::clamp(value, 100, 900);
    return true;
  } catch (...) {
    return false;
  }
}

bool AppendFamily(std::string_view family, std::vector<std::string>* families) {
  family = Trim(family);
  if (family.empty()) {
    return false;
  }

  if (family.front() == '\'' || family.front() == '"') {
    if (family.size() < 2 || family.back() != family.front()) {
      return false;
    }
    family = Trim(family.substr(1, family.size() - 2));
    if (family.empty()) {
      return false;
    }
    families->emplace_back(family);
    return true;
  }

  std::string normalized;
  bool pending_space = false;
  for (unsigned char ch : family) {
    if (ch == '\'' || ch == '"') {
      return false;
    }
    if (std::isspace(ch)) {
      pending_space = !normalized.empty();
      continue;
    }
    if (pending_space) {
      normalized.push_back(' ');
      pending_space = false;
    }
    normalized.push_back(static_cast<char>(ch));
  }
  if (normalized.empty()) {
    return false;
  }
  families->push_back(std::move(normalized));
  return true;
}

bool ParseFamilies(std::string_view value,
                   std::vector<std::string>* families) {
  char quote = 0;
  std::size_t start = 0;
  for (std::size_t i = 0; i < value.size(); ++i) {
    const char ch = value[i];
    if (quote != 0) {
      if (ch == '\\' && i + 1 < value.size()) {
        ++i;
      } else if (ch == quote) {
        quote = 0;
      }
    } else if (ch == '\'' || ch == '"') {
      quote = ch;
    } else if (ch == ',') {
      if (!AppendFamily(value.substr(start, i - start), families)) {
        return false;
      }
      start = i + 1;
    }
  }
  if (quote != 0 ||
      !AppendFamily(value.substr(start), families)) {
    return false;
  }
  return !families->empty();
}

bool ParsePrefixToken(std::string_view token, FontDescriptor* descriptor,
                      bool* style_set, bool* weight_set) {
  const std::string lower = ToLower(token);
  if (lower == "normal") {
    if (!*style_set) {
      descriptor->slant = SkFontStyle::kUpright_Slant;
      *style_set = true;
    } else if (!*weight_set) {
      descriptor->weight = 400;
      *weight_set = true;
    }
    return true;
  }
  if (lower == "italic" || lower == "oblique") {
    if (*style_set) {
      return false;
    }
    descriptor->slant = lower == "italic" ? SkFontStyle::kItalic_Slant
                                          : SkFontStyle::kOblique_Slant;
    *style_set = true;
    return true;
  }
  if (lower == "small-caps" || IsStretchKeyword(lower)) {
    return true;
  }
  if (!*weight_set && ParseWeight(token, &descriptor->weight)) {
    *weight_set = true;
    return true;
  }
  return false;
}

}  // namespace

std::optional<FontDescriptor> ParseCssFont(std::string_view css) {
  const auto tokens = Tokenize(css);
  if (tokens.empty()) {
    return std::nullopt;
  }

  FontDescriptor descriptor;
  descriptor.families.clear();
  bool style_set = false;
  bool weight_set = false;
  for (const auto& token : tokens) {
    if (ParseSizeToken(token.value, &descriptor.size_px)) {
      const auto families = Trim(css.substr(token.end));
      if (families.empty() ||
          !ParseFamilies(families, &descriptor.families)) {
        return std::nullopt;
      }
      return descriptor;
    }
    if (!ParsePrefixToken(token.value, &descriptor, &style_set, &weight_set)) {
      return std::nullopt;
    }
  }

  return std::nullopt;
}

}  // namespace canvas_engine
