#include "canvas_engine/canvas/Canvas2DContext.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <optional>
#include <string>

#include "include/core/SkBlendMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkScalar.h"
#include "include/effects/SkImageFilters.h"

#include "canvas_engine/canvas/ColorParser.h"

namespace canvas_engine {

namespace {

constexpr float kRadiansToDegrees = 57.29577951308232f;

std::string ToLower(std::string_view value) {
  std::string lower(value);
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return lower;
}

std::optional<float> ParseFontSize(std::string_view font) {
  const std::string css = ToLower(font);
  const std::size_t px = css.find("px");
  if (px == std::string::npos) {
    return std::nullopt;
  }

  std::size_t start = px;
  while (start > 0) {
    const char ch = css[start - 1];
    if ((ch >= '0' && ch <= '9') || ch == '.') {
      --start;
      continue;
    }
    break;
  }

  if (start == px) {
    return std::nullopt;
  }

  try {
    return std::stof(css.substr(start, px - start));
  } catch (...) {
    return std::nullopt;
  }
}

float ComputeShadowSigma(float blur) {
  return std::max(0.0f, blur * 0.5f);
}

}  // namespace

Canvas2DContext::Canvas2DContext(std::shared_ptr<CanvasSurface> surface)
    : surface_(std::move(surface)) {}

Canvas2DContext::State Canvas2DContext::MakeDefaultState() {
  State state;
  state.shadow_color = SkColorSetARGB(0, 0, 0, 0);
  return state;
}

bool Canvas2DContext::SetFillStyle(std::string_view css_color) {
  auto parsed = ParseCssColor(css_color);
  if (!parsed.has_value()) {
    return false;
  }
  state_.fill_style = parsed.value();
  state_.fill_style_css = NormalizeCssColor(css_color);
  return true;
}

bool Canvas2DContext::SetStrokeStyle(std::string_view css_color) {
  auto parsed = ParseCssColor(css_color);
  if (!parsed.has_value()) {
    return false;
  }
  state_.stroke_style = parsed.value();
  state_.stroke_style_css = NormalizeCssColor(css_color);
  return true;
}

bool Canvas2DContext::SetLineWidth(float value) {
  if (!(value > 0.0f)) {
    return false;
  }
  state_.line_width = value;
  return true;
}

bool Canvas2DContext::SetFont(std::string_view font) {
  auto size = ParseFontSize(font);
  if (!size.has_value() || !(size.value() > 0.0f)) {
    return false;
  }
  state_.font_css = std::string(font);
  state_.font_size = size.value();
  return true;
}

bool Canvas2DContext::SetTextAlign(std::string_view value) {
  const std::string align = ToLower(value);
  if (align != "left" && align != "right" && align != "center" &&
      align != "start" && align != "end") {
    return false;
  }
  state_.text_align = align;
  return true;
}

bool Canvas2DContext::SetTextBaseline(std::string_view value) {
  const std::string baseline = ToLower(value);
  if (baseline != "top" && baseline != "hanging" && baseline != "middle" &&
      baseline != "alphabetic" && baseline != "ideographic" &&
      baseline != "bottom") {
    return false;
  }
  state_.text_baseline = baseline;
  return true;
}

bool Canvas2DContext::SetGlobalAlpha(float value) {
  if (std::isnan(value)) {
    return false;
  }
  state_.global_alpha = std::clamp(value, 0.0f, 1.0f);
  return true;
}

bool Canvas2DContext::SetGlobalCompositeOperation(std::string_view value) {
  const std::string op = ToLower(value);
  static constexpr std::array<std::pair<std::string_view, SkBlendMode>, 26>
      kBlendModes = {{
          {"source-over", SkBlendMode::kSrcOver},
          {"source-in", SkBlendMode::kSrcIn},
          {"source-out", SkBlendMode::kSrcOut},
          {"source-atop", SkBlendMode::kSrcATop},
          {"destination-over", SkBlendMode::kDstOver},
          {"destination-in", SkBlendMode::kDstIn},
          {"destination-out", SkBlendMode::kDstOut},
          {"destination-atop", SkBlendMode::kDstATop},
          {"lighter", SkBlendMode::kPlus},
          {"copy", SkBlendMode::kSrc},
          {"xor", SkBlendMode::kXor},
          {"multiply", SkBlendMode::kMultiply},
          {"screen", SkBlendMode::kScreen},
          {"overlay", SkBlendMode::kOverlay},
          {"darken", SkBlendMode::kDarken},
          {"lighten", SkBlendMode::kLighten},
          {"color-dodge", SkBlendMode::kColorDodge},
          {"color-burn", SkBlendMode::kColorBurn},
          {"hard-light", SkBlendMode::kHardLight},
          {"soft-light", SkBlendMode::kSoftLight},
          {"difference", SkBlendMode::kDifference},
          {"exclusion", SkBlendMode::kExclusion},
          {"hue", SkBlendMode::kHue},
          {"saturation", SkBlendMode::kSaturation},
          {"color", SkBlendMode::kColor},
          {"luminosity", SkBlendMode::kLuminosity},
      }};

  for (const auto& [name, mode] : kBlendModes) {
    if (op == name) {
      state_.global_composite_operation = op;
      state_.blend_mode = mode;
      return true;
    }
  }
  return false;
}

bool Canvas2DContext::SetLineCap(std::string_view value) {
  const std::string cap = ToLower(value);
  if (cap == "butt") {
    state_.line_cap = cap;
    state_.stroke_cap = SkPaint::kButt_Cap;
    return true;
  }
  if (cap == "round") {
    state_.line_cap = cap;
    state_.stroke_cap = SkPaint::kRound_Cap;
    return true;
  }
  if (cap == "square") {
    state_.line_cap = cap;
    state_.stroke_cap = SkPaint::kSquare_Cap;
    return true;
  }
  return false;
}

bool Canvas2DContext::SetLineJoin(std::string_view value) {
  const std::string join = ToLower(value);
  if (join == "miter") {
    state_.line_join = join;
    state_.stroke_join = SkPaint::kMiter_Join;
    return true;
  }
  if (join == "round") {
    state_.line_join = join;
    state_.stroke_join = SkPaint::kRound_Join;
    return true;
  }
  if (join == "bevel") {
    state_.line_join = join;
    state_.stroke_join = SkPaint::kBevel_Join;
    return true;
  }
  return false;
}

bool Canvas2DContext::SetMiterLimit(float value) {
  if (!(value > 0.0f)) {
    return false;
  }
  state_.miter_limit = value;
  return true;
}

bool Canvas2DContext::SetShadowBlur(float value) {
  if (value < 0.0f || std::isnan(value)) {
    return false;
  }
  state_.shadow_blur = value;
  return true;
}

bool Canvas2DContext::SetShadowColor(std::string_view css_color) {
  auto parsed = ParseCssColor(css_color);
  if (!parsed.has_value()) {
    return false;
  }
  state_.shadow_color = parsed.value();
  state_.shadow_color_css = NormalizeCssColor(css_color);
  return true;
}

void Canvas2DContext::SetShadowOffsetX(float value) {
  state_.shadow_offset_x = value;
}

void Canvas2DContext::SetShadowOffsetY(float value) {
  state_.shadow_offset_y = value;
}

void Canvas2DContext::ResetState() {
  state_ = MakeDefaultState();
  state_stack_.clear();
  current_path_.reset();
}

void Canvas2DContext::Save() {
  state_stack_.push_back(state_);
  surface_->canvas()->save();
}

void Canvas2DContext::Restore() {
  if (state_stack_.empty()) {
    return;
  }
  state_ = state_stack_.back();
  state_stack_.pop_back();
  surface_->canvas()->restore();
}

void Canvas2DContext::Translate(float dx, float dy) {
  surface_->canvas()->translate(dx, dy);
}

void Canvas2DContext::Scale(float sx, float sy) {
  surface_->canvas()->scale(sx, sy);
}

void Canvas2DContext::Rotate(float radians) {
  surface_->canvas()->rotate(radians * kRadiansToDegrees);
}

void Canvas2DContext::Transform(float a, float b, float c, float d, float e,
                                float f) {
  const SkMatrix matrix = SkMatrix::MakeAll(a, c, e, b, d, f, 0.0f, 0.0f, 1.0f);
  surface_->canvas()->concat(matrix);
}

void Canvas2DContext::SetTransform(float a, float b, float c, float d, float e,
                                   float f) {
  surface_->canvas()->resetMatrix();
  Transform(a, b, c, d, e, f);
}

void Canvas2DContext::ResetTransform() {
  surface_->canvas()->resetMatrix();
}

void Canvas2DContext::ClearRect(float x, float y, float width, float height) {
  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kClear);
  surface_->canvas()->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);
}

void Canvas2DContext::FillRect(float x, float y, float width, float height) {
  surface_->canvas()->drawRect(SkRect::MakeXYWH(x, y, width, height), MakeFillPaint());
}

void Canvas2DContext::StrokeRect(float x, float y, float width, float height) {
  surface_->canvas()->drawRect(SkRect::MakeXYWH(x, y, width, height), MakeStrokePaint());
}

void Canvas2DContext::BeginPath() {
  current_path_.reset();
}

void Canvas2DContext::MoveTo(float x, float y) {
  current_path_.moveTo(x, y);
}

void Canvas2DContext::LineTo(float x, float y) {
  current_path_.lineTo(x, y);
}

void Canvas2DContext::Rect(float x, float y, float width, float height) {
  current_path_.addRect(SkRect::MakeXYWH(x, y, width, height));
}

void Canvas2DContext::Arc(float x, float y, float radius, float start_angle,
                          float end_angle, bool counter_clockwise) {
  if (radius <= 0.0f) {
    return;
  }

  float sweep = (end_angle - start_angle) * kRadiansToDegrees;
  if (!counter_clockwise && sweep < 0.0f) {
    sweep += 360.0f;
  } else if (counter_clockwise && sweep > 0.0f) {
    sweep -= 360.0f;
  }

  SkRect oval = SkRect::MakeLTRB(x - radius, y - radius, x + radius, y + radius);
  current_path_.arcTo(oval, start_angle * kRadiansToDegrees, sweep, false);
}

void Canvas2DContext::ClosePath() {
  current_path_.close();
}

void Canvas2DContext::Fill() {
  surface_->canvas()->drawPath(current_path_.snapshot(), MakeFillPaint());
}

void Canvas2DContext::Stroke() {
  surface_->canvas()->drawPath(current_path_.snapshot(), MakeStrokePaint());
}

Canvas2DContext::TextMetrics Canvas2DContext::MeasureText(
    std::string_view text) const {
  TextMetrics metrics;
  const SkFont font = MakeFont();
  SkRect bounds;
  metrics.width = font.measureText(text.data(), text.size(),
                                   SkTextEncoding::kUTF8, &bounds);

  SkFontMetrics font_metrics;
  font.getMetrics(&font_metrics);
  metrics.actual_bounding_box_ascent = std::max(-font_metrics.fAscent, 0.0f);
  metrics.actual_bounding_box_descent = std::max(font_metrics.fDescent, 0.0f);
  return metrics;
}

void Canvas2DContext::FillText(std::string_view text, float x, float y) {
  DrawText(text, x, y, MakeTextPaint(false));
}

void Canvas2DContext::StrokeText(std::string_view text, float x, float y) {
  DrawText(text, x, y, MakeTextPaint(true));
}

SkFont Canvas2DContext::MakeFont() const {
  SkFont font;
  font.setSize(state_.font_size);
  font.setSubpixel(true);
  font.setEdging(SkFont::Edging::kAntiAlias);
  return font;
}

SkPaint Canvas2DContext::MakeFillPaint() const {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(state_.fill_style);
  ConfigurePaint(&paint);
  return paint;
}

SkPaint Canvas2DContext::MakeStrokePaint() const {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(state_.stroke_style);
  paint.setStrokeWidth(state_.line_width);
  ConfigurePaint(&paint);
  return paint;
}

SkPaint Canvas2DContext::MakeTextPaint(bool stroke) const {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(stroke ? SkPaint::kStroke_Style : SkPaint::kFill_Style);
  paint.setColor(stroke ? state_.stroke_style : state_.fill_style);
  paint.setStrokeWidth(state_.line_width);
  ConfigurePaint(&paint);
  return paint;
}

void Canvas2DContext::ConfigurePaint(SkPaint* paint) const {
  paint->setAlphaf(state_.global_alpha);
  paint->setBlendMode(state_.blend_mode);
  paint->setStrokeCap(state_.stroke_cap);
  paint->setStrokeJoin(state_.stroke_join);
  paint->setStrokeMiter(state_.miter_limit);

  const bool has_shadow =
      (SkColorGetA(state_.shadow_color) > 0) &&
      (state_.shadow_blur > 0.0f || state_.shadow_offset_x != 0.0f ||
       state_.shadow_offset_y != 0.0f);
  if (has_shadow) {
    const float sigma = ComputeShadowSigma(state_.shadow_blur);
    paint->setImageFilter(SkImageFilters::DropShadow(
        state_.shadow_offset_x, state_.shadow_offset_y, sigma, sigma,
        state_.shadow_color, nullptr));
  } else {
    paint->setImageFilter(nullptr);
  }
}

void Canvas2DContext::DrawText(std::string_view text, float x, float y,
                               const SkPaint& paint) const {
  const SkFont font = MakeFont();
  SkFontMetrics metrics;
  font.getMetrics(&metrics);
  const TextMetrics text_metrics = MeasureText(text);

  float draw_x = x;
  if (state_.text_align == "center") {
    draw_x -= text_metrics.width * 0.5f;
  } else if (state_.text_align == "right" || state_.text_align == "end") {
    draw_x -= text_metrics.width;
  }

  float baseline_y = y;
  if (state_.text_baseline == "top" || state_.text_baseline == "hanging") {
    baseline_y -= metrics.fAscent;
  } else if (state_.text_baseline == "middle") {
    baseline_y -= (metrics.fAscent + metrics.fDescent) * 0.5f;
  } else if (state_.text_baseline == "bottom" ||
             state_.text_baseline == "ideographic") {
    baseline_y -= metrics.fDescent;
  }

  surface_->canvas()->drawSimpleText(text.data(), text.size(),
                                     SkTextEncoding::kUTF8, draw_x, baseline_y,
                                     font, paint);
}

}  // namespace canvas_engine
