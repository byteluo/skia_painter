#include "canvas_engine/canvas/Canvas2DContext.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "include/core/SkBlendMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkScalar.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkTypeface.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkImageFilters.h"

#include "canvas_engine/canvas/ColorParser.h"

namespace canvas_engine {

namespace {

constexpr float kRadiansToDegrees = 57.29577951308232f;
constexpr float kFullCircleDegrees = 360.0f;
constexpr float kFullCircleEpsilon = 0.001f;

std::string ToLower(std::string_view value) {
  std::string lower(value);
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return lower;
}

float ComputeShadowSigma(float blur) {
  return std::max(0.0f, blur * 0.5f);
}

float BaselineOffset(std::string_view baseline, float ascent, float descent) {
  if (baseline == "top") {
    return ascent;
  }
  if (baseline == "hanging") {
    return ascent * 0.8f;
  }
  if (baseline == "middle") {
    return (ascent - descent) * 0.5f;
  }
  if (baseline == "ideographic") {
    return -descent * 0.5f;
  }
  if (baseline == "bottom") {
    return -descent;
  }
  return 0.0f;
}

std::uint8_t ClampByte(int value) {
  return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

std::vector<SkUnichar> DecodeUtf8(std::string_view text) {
  std::vector<SkUnichar> codepoints;
  for (std::size_t index = 0; index < text.size();) {
    const auto first = static_cast<std::uint8_t>(text[index]);
    if (first < 0x80) {
      codepoints.push_back(first);
      ++index;
      continue;
    }

    int length = 0;
    SkUnichar codepoint = 0;
    if ((first & 0xe0) == 0xc0) {
      length = 2;
      codepoint = first & 0x1f;
    } else if ((first & 0xf0) == 0xe0) {
      length = 3;
      codepoint = first & 0x0f;
    } else if ((first & 0xf8) == 0xf0 && first <= 0xf4) {
      length = 4;
      codepoint = first & 0x07;
    } else {
      codepoints.push_back(0xfffd);
      ++index;
      continue;
    }

    bool valid = index + static_cast<std::size_t>(length) <= text.size();
    for (int offset = 1; valid && offset < length; ++offset) {
      const auto next =
          static_cast<std::uint8_t>(text[index + static_cast<std::size_t>(offset)]);
      if ((next & 0xc0) != 0x80) {
        valid = false;
        break;
      }
      codepoint = (codepoint << 6) | (next & 0x3f);
    }
    if (valid &&
        ((length == 2 && codepoint < 0x80) ||
         (length == 3 && codepoint < 0x800) ||
         (length == 4 && codepoint < 0x10000) ||
         (codepoint >= 0xd800 && codepoint <= 0xdfff) ||
         codepoint > 0x10ffff)) {
      valid = false;
    }
    if (!valid) {
      codepoints.push_back(0xfffd);
      ++index;
      continue;
    }
    codepoints.push_back(codepoint);
    index += static_cast<std::size_t>(length);
  }
  return codepoints;
}

SkColor ApplyGlobalAlpha(SkColor color, float global_alpha) {
  const float alpha =
      (static_cast<float>(SkColorGetA(color)) / 255.0f) * global_alpha;
  const auto alpha_byte = static_cast<U8CPU>(
      std::lround(std::clamp(alpha, 0.0f, 1.0f) * 255.0f));
  return SkColorSetARGB(alpha_byte, SkColorGetR(color), SkColorGetG(color),
                        SkColorGetB(color));
}

}  // namespace

Canvas2DContext::Canvas2DContext(
    std::shared_ptr<CanvasSurface> surface,
    std::function<void()> draw_op_callback,
    std::shared_ptr<FontManager> font_manager)
    : surface_(std::move(surface)),
      draw_op_callback_(std::move(draw_op_callback)),
      font_manager_(std::move(font_manager)) {
  ApplyDeviceScale();
}

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
  state_.fill_source.reset();
  state_.fill_style_css = NormalizeCssColor(css_color);
  return true;
}

bool Canvas2DContext::SetStrokeStyle(std::string_view css_color) {
  auto parsed = ParseCssColor(css_color);
  if (!parsed.has_value()) {
    return false;
  }
  state_.stroke_style = parsed.value();
  state_.stroke_source.reset();
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
  auto descriptor = ParseCssFont(font);
  if (!descriptor.has_value()) {
    return false;
  }
  state_.font_css = std::string(font);
  state_.font_descriptor = std::move(descriptor.value());
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

void Canvas2DContext::SetFillSource(
    std::shared_ptr<const PaintSource> source) {
  if (!source) {
    return;
  }
  state_.fill_style_css = source->description();
  state_.fill_source = std::move(source);
}

void Canvas2DContext::SetStrokeSource(
    std::shared_ptr<const PaintSource> source) {
  if (!source) {
    return;
  }
  state_.stroke_style_css = source->description();
  state_.stroke_source = std::move(source);
}

void Canvas2DContext::SetLineDash(const std::vector<float>& segments) {
  state_.line_dash.clear();
  for (float segment : segments) {
    if (segment > 0.0f && !std::isnan(segment)) {
      state_.line_dash.push_back(segment);
    }
  }
}

void Canvas2DContext::SetLineDashOffset(float value) {
  if (!std::isnan(value)) {
    state_.line_dash_offset = value;
  }
}

void Canvas2DContext::ResetState() {
  state_ = MakeDefaultState();
  state_stack_.clear();
  current_path_.reset();
  surface_->canvas()->resetMatrix();
  ApplyDeviceScale();
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
  ApplyDeviceScale();
  Transform(a, b, c, d, e, f);
}

void Canvas2DContext::ResetTransform() {
  surface_->canvas()->resetMatrix();
  ApplyDeviceScale();
}

void Canvas2DContext::ClearRect(float x, float y, float width, float height) {
  RecordDrawOp();
  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kClear);
  surface_->canvas()->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);
}

void Canvas2DContext::FillRect(float x, float y, float width, float height) {
  RecordDrawOp();
  surface_->canvas()->drawRect(SkRect::MakeXYWH(x, y, width, height), MakeFillPaint());
}

void Canvas2DContext::StrokeRect(float x, float y, float width, float height) {
  RecordDrawOp();
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

void Canvas2DContext::QuadraticCurveTo(float cpx, float cpy, float x, float y) {
  current_path_.quadTo(cpx, cpy, x, y);
}

void Canvas2DContext::BezierCurveTo(float cp1x, float cp1y, float cp2x,
                                    float cp2y, float x, float y) {
  current_path_.cubicTo(cp1x, cp1y, cp2x, cp2y, x, y);
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
  const float start_degrees = start_angle * kRadiansToDegrees;
  if (std::fabs(sweep) >= kFullCircleDegrees - kFullCircleEpsilon) {
    const float half_sweep = counter_clockwise ? -180.0f : 180.0f;
    current_path_.arcTo(oval, start_degrees, half_sweep, false);
    current_path_.arcTo(oval, start_degrees + half_sweep, half_sweep, false);
    return;
  }

  current_path_.arcTo(oval, start_degrees, sweep, false);
}

void Canvas2DContext::ArcTo(float x1, float y1, float x2, float y2, float radius) {
  if (radius < 0.0f) {
    return;
  }
  current_path_.arcTo(SkPoint::Make(x1, y1), SkPoint::Make(x2, y2), radius);
}

void Canvas2DContext::Ellipse(float x, float y, float radius_x, float radius_y,
                              float rotation, float start_angle, float end_angle,
                              bool counter_clockwise) {
  if (!(radius_x > 0.0f) || !(radius_y > 0.0f)) {
    return;
  }

  float sweep = (end_angle - start_angle) * kRadiansToDegrees;
  if (!counter_clockwise && sweep < 0.0f) {
    sweep += 360.0f;
  } else if (counter_clockwise && sweep > 0.0f) {
    sweep -= 360.0f;
  }

  SkPathBuilder ellipse_path;
  if (std::fabs(sweep) >= kFullCircleDegrees - kFullCircleEpsilon) {
    ellipse_path.addOval(SkRect::MakeLTRB(-1.0f, -1.0f, 1.0f, 1.0f),
                         counter_clockwise ? SkPathDirection::kCCW
                                           : SkPathDirection::kCW);
  } else {
    ellipse_path.arcTo(SkRect::MakeLTRB(-1.0f, -1.0f, 1.0f, 1.0f),
                       start_angle * kRadiansToDegrees, sweep, false);
  }

  const float sin_rotation = std::sin(rotation);
  const float cos_rotation = std::cos(rotation);
  const SkMatrix transform = SkMatrix::MakeAll(
      cos_rotation * radius_x, -sin_rotation * radius_y, x,
      sin_rotation * radius_x, cos_rotation * radius_y, y,
      0.0f, 0.0f, 1.0f);
  current_path_.addPath(ellipse_path.snapshot(), transform);
}

void Canvas2DContext::ClosePath() {
  current_path_.close();
}

void Canvas2DContext::Clip(SkPathFillType fill_type) {
  Clip(current_path_.snapshot(), fill_type);
}

void Canvas2DContext::Clip(const SkPath& source_path,
                           SkPathFillType fill_type) {
  SkPath path = source_path;
  path.setFillType(fill_type);
  surface_->canvas()->clipPath(path, true);
}

void Canvas2DContext::Fill(SkPathFillType fill_type) {
  Fill(current_path_.snapshot(), fill_type);
}

void Canvas2DContext::Fill(const SkPath& source_path,
                           SkPathFillType fill_type) {
  RecordDrawOp();
  SkPath path = source_path;
  path.setFillType(fill_type);
  surface_->canvas()->drawPath(path, MakeFillPaint());
}

void Canvas2DContext::Stroke() {
  Stroke(current_path_.snapshot());
}

void Canvas2DContext::Stroke(const SkPath& path) {
  RecordDrawOp();
  surface_->canvas()->drawPath(path, MakeStrokePaint());
}

bool Canvas2DContext::IsPointInPath(float x, float y,
                                    SkPathFillType fill_type) const {
  return IsPointInPath(current_path_.snapshot(), x, y, fill_type);
}

bool Canvas2DContext::IsPointInPath(const SkPath& source_path, float x,
                                    float y,
                                    SkPathFillType fill_type) const {
  SkPath path = source_path;
  path.setFillType(fill_type);
  return path.contains(x, y);
}

void Canvas2DContext::DrawImage(const sk_sp<SkImage>& image, float sx, float sy,
                                float sw, float sh, float dx, float dy, float dw,
                                float dh) {
  if (!image || !(sw > 0.0f) || !(sh > 0.0f) || !(dw > 0.0f) || !(dh > 0.0f)) {
    return;
  }

  RecordDrawOp();
  const SkRect src = SkRect::MakeXYWH(sx, sy, sw, sh);
  const SkRect dst = SkRect::MakeXYWH(dx, dy, dw, dh);
  surface_->canvas()->drawImageRect(image, src, dst,
                                    SkSamplingOptions(SkFilterMode::kLinear),
                                    nullptr, SkCanvas::kStrict_SrcRectConstraint);
}

std::optional<Canvas2DContext::ImageData> Canvas2DContext::GetImageData(
    int x, int y, int width, int height) const {
  if (width <= 0 || height <= 0) {
    return std::nullopt;
  }

  SkPixmap pixmap;
  if (!surface_->PeekPixels(&pixmap)) {
    return std::nullopt;
  }

  ImageData result;
  result.width = width;
  result.height = height;
  result.data.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u, 0);

  const float pixel_ratio = surface_->pixel_ratio();
  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < width; ++col) {
      const int device_left = static_cast<int>(std::floor((x + col) * pixel_ratio));
      const int device_top = static_cast<int>(std::floor((y + row) * pixel_ratio));
      const int device_right = static_cast<int>(std::ceil((x + col + 1) * pixel_ratio));
      const int device_bottom = static_cast<int>(std::ceil((y + row + 1) * pixel_ratio));

      const int left = std::clamp(device_left, 0, pixmap.width());
      const int top = std::clamp(device_top, 0, pixmap.height());
      const int right = std::clamp(device_right, 0, pixmap.width());
      const int bottom = std::clamp(device_bottom, 0, pixmap.height());
      if (left >= right || top >= bottom) {
        continue;
      }

      std::uint64_t red = 0;
      std::uint64_t green = 0;
      std::uint64_t blue = 0;
      std::uint64_t alpha = 0;
      int samples = 0;

      for (int yy = top; yy < bottom; ++yy) {
        for (int xx = left; xx < right; ++xx) {
          const auto* pixel =
              static_cast<const std::uint8_t*>(pixmap.addr(xx, yy));
          if (pixel == nullptr) {
            continue;
          }

          if (pixmap.colorType() == kBGRA_8888_SkColorType) {
            blue += pixel[0];
            green += pixel[1];
            red += pixel[2];
            alpha += pixel[3];
          } else {
            red += pixel[0];
            green += pixel[1];
            blue += pixel[2];
            alpha += pixel[3];
          }
          ++samples;
        }
      }

      if (samples == 0) {
        continue;
      }

      const std::uint8_t avg_alpha = static_cast<std::uint8_t>(alpha / samples);
      std::uint8_t avg_red = static_cast<std::uint8_t>(red / samples);
      std::uint8_t avg_green = static_cast<std::uint8_t>(green / samples);
      std::uint8_t avg_blue = static_cast<std::uint8_t>(blue / samples);
      if (avg_alpha != 0 && avg_alpha != 255) {
        avg_red = ClampByte((static_cast<int>(avg_red) * 255 + avg_alpha / 2) / avg_alpha);
        avg_green = ClampByte((static_cast<int>(avg_green) * 255 + avg_alpha / 2) / avg_alpha);
        avg_blue = ClampByte((static_cast<int>(avg_blue) * 255 + avg_alpha / 2) / avg_alpha);
      }

      const size_t offset =
          (static_cast<size_t>(row) * static_cast<size_t>(width) + static_cast<size_t>(col)) * 4u;
      result.data[offset + 0] = avg_red;
      result.data[offset + 1] = avg_green;
      result.data[offset + 2] = avg_blue;
      result.data[offset + 3] = avg_alpha;
    }
  }

  return result;
}

bool Canvas2DContext::PutImageData(const ImageData& image_data, int dx, int dy) {
  if (image_data.width <= 0 || image_data.height <= 0) {
    return false;
  }

  const size_t expected_size =
      static_cast<size_t>(image_data.width) * static_cast<size_t>(image_data.height) * 4u;
  if (image_data.data.size() != expected_size) {
    return false;
  }

  SkPixmap pixmap;
  if (!surface_->PeekPixels(&pixmap)) {
    return false;
  }

  RecordDrawOp();
  const float pixel_ratio = surface_->pixel_ratio();
  for (int row = 0; row < image_data.height; ++row) {
    for (int col = 0; col < image_data.width; ++col) {
      const size_t offset =
          (static_cast<size_t>(row) * static_cast<size_t>(image_data.width) +
           static_cast<size_t>(col)) * 4u;
      const std::uint8_t src_red = image_data.data[offset + 0];
      const std::uint8_t src_green = image_data.data[offset + 1];
      const std::uint8_t src_blue = image_data.data[offset + 2];
      const std::uint8_t src_alpha = image_data.data[offset + 3];

      const std::uint8_t premul_red =
          static_cast<std::uint8_t>((static_cast<int>(src_red) * src_alpha + 127) / 255);
      const std::uint8_t premul_green =
          static_cast<std::uint8_t>((static_cast<int>(src_green) * src_alpha + 127) / 255);
      const std::uint8_t premul_blue =
          static_cast<std::uint8_t>((static_cast<int>(src_blue) * src_alpha + 127) / 255);

      const int device_left = static_cast<int>(std::floor((dx + col) * pixel_ratio));
      const int device_top = static_cast<int>(std::floor((dy + row) * pixel_ratio));
      const int device_right = static_cast<int>(std::ceil((dx + col + 1) * pixel_ratio));
      const int device_bottom = static_cast<int>(std::ceil((dy + row + 1) * pixel_ratio));

      const int left = std::clamp(device_left, 0, pixmap.width());
      const int top = std::clamp(device_top, 0, pixmap.height());
      const int right = std::clamp(device_right, 0, pixmap.width());
      const int bottom = std::clamp(device_bottom, 0, pixmap.height());
      if (left >= right || top >= bottom) {
        continue;
      }

      for (int yy = top; yy < bottom; ++yy) {
        for (int xx = left; xx < right; ++xx) {
          auto* pixel = static_cast<std::uint8_t*>(pixmap.writable_addr(xx, yy));
          if (pixel == nullptr) {
            continue;
          }

          if (pixmap.colorType() == kBGRA_8888_SkColorType) {
            pixel[0] = premul_blue;
            pixel[1] = premul_green;
            pixel[2] = premul_red;
            pixel[3] = src_alpha;
          } else {
            pixel[0] = premul_red;
            pixel[1] = premul_green;
            pixel[2] = premul_blue;
            pixel[3] = src_alpha;
          }
        }
      }
    }
  }

  return true;
}

Canvas2DContext::TextMetrics Canvas2DContext::MeasureText(
    std::string_view text) const {
  return MeasureTextRuns(MakeTextRuns(text));
}

Canvas2DContext::TextMetrics Canvas2DContext::MeasureTextRuns(
    const std::vector<TextRun>& runs) const {
  TextMetrics metrics;
  float font_ascent = 0.0f;
  float font_descent = 0.0f;
  if (runs.empty()) {
    SkFontMetrics font_metrics;
    MakeFont().getMetrics(&font_metrics);
    font_ascent = std::max(-font_metrics.fAscent, 0.0f);
    font_descent = std::max(font_metrics.fDescent, 0.0f);
  }
  float pen_x = 0.0f;
  float left = 0.0f;
  float right = 0.0f;
  float top = 0.0f;
  float bottom = 0.0f;
  bool has_bounds = false;
  for (const auto& run : runs) {
    SkFontMetrics run_metrics;
    run.font.getMetrics(&run_metrics);
    font_ascent = std::max(font_ascent, std::max(-run_metrics.fAscent, 0.0f));
    font_descent =
        std::max(font_descent, std::max(run_metrics.fDescent, 0.0f));
    for (const SkGlyphID glyph : run.glyphs) {
      const float advance = run.font.getWidth(glyph);
      const SkRect bounds = run.font.getBounds(glyph, nullptr);
      if (!bounds.isEmpty()) {
        if (!has_bounds) {
          left = pen_x + bounds.left();
          right = pen_x + bounds.right();
          top = bounds.top();
          bottom = bounds.bottom();
          has_bounds = true;
        } else {
          left = std::min(left, pen_x + bounds.left());
          right = std::max(right, pen_x + bounds.right());
          top = std::min(top, bounds.top());
          bottom = std::max(bottom, bounds.bottom());
        }
      }
      pen_x += advance;
    }
  }
  metrics.width = pen_x;
  metrics.baseline_offset =
      BaselineOffset(state_.text_baseline, font_ascent, font_descent);
  metrics.font_bounding_box_ascent =
      std::max(font_ascent - metrics.baseline_offset, 0.0f);
  metrics.font_bounding_box_descent =
      std::max(font_descent + metrics.baseline_offset, 0.0f);
  if (has_bounds) {
    float alignment_offset = 0.0f;
    if (state_.text_align == "center") {
      alignment_offset = -metrics.width * 0.5f;
    } else if (state_.text_align == "right" ||
               state_.text_align == "end") {
      alignment_offset = -metrics.width;
    }
    metrics.actual_bounding_box_left = -(alignment_offset + left);
    metrics.actual_bounding_box_right = alignment_offset + right;
    metrics.actual_bounding_box_ascent =
        std::max(-(metrics.baseline_offset + top), 0.0f);
    metrics.actual_bounding_box_descent =
        std::max(metrics.baseline_offset + bottom, 0.0f);
  }
  metrics.em_height_ascent = metrics.font_bounding_box_ascent;
  metrics.em_height_descent = metrics.font_bounding_box_descent;
  metrics.alphabetic_baseline = -metrics.baseline_offset;
  metrics.hanging_baseline =
      BaselineOffset("hanging", font_ascent, font_descent) -
      metrics.baseline_offset;
  metrics.ideographic_baseline =
      BaselineOffset("ideographic", font_ascent, font_descent) -
      metrics.baseline_offset;
  return metrics;
}

void Canvas2DContext::FillText(std::string_view text, float x, float y) {
  DrawText(text, x, y, MakeTextPaint(false));
}

void Canvas2DContext::StrokeText(std::string_view text, float x, float y) {
  DrawText(text, x, y, MakeTextPaint(true));
}

void Canvas2DContext::ApplyDeviceScale() {
  if (!surface_) {
    return;
  }

  const float pixel_ratio = surface_->pixel_ratio();
  if (pixel_ratio != 1.0f) {
    surface_->canvas()->scale(pixel_ratio, pixel_ratio);
  }
}

SkFont Canvas2DContext::MakeFont() const {
  const auto& descriptor = state_.font_descriptor;
  const SkFontStyle requested_style(
      descriptor.weight, SkFontStyle::kNormal_Width, descriptor.slant);
  if (font_manager_) {
    sk_sp<SkTypeface> typeface;
    for (const auto& family : descriptor.families) {
      typeface = font_manager_->Match(family, requested_style);
      if (typeface) {
        break;
      }
    }
    if (!typeface) {
      typeface = font_manager_->Match("Noto Sans", requested_style);
    }
    return MakeFontForTypeface(std::move(typeface));
  }
  return MakeFontForTypeface(nullptr);
}

SkFont Canvas2DContext::MakeFontForTypeface(
    sk_sp<SkTypeface> typeface) const {
  SkFont font;
  const auto& descriptor = state_.font_descriptor;
  font.setSize(descriptor.size_px);
  font.setSubpixel(true);
  font.setEdging(SkFont::Edging::kAntiAlias);
  if (typeface) {
    const SkFontStyle matched_style = typeface->fontStyle();
    font.setTypeface(std::move(typeface));
    if (descriptor.weight >= 600 && matched_style.weight() < 600) {
      font.setEmbolden(true);
    }
    if (descriptor.slant != SkFontStyle::kUpright_Slant &&
        matched_style.slant() == SkFontStyle::kUpright_Slant) {
      font.setSkewX(-0.25f);
    }
  }
  return font;
}

std::vector<Canvas2DContext::TextRun> Canvas2DContext::MakeTextRuns(
    std::string_view text) const {
  std::vector<TextRun> runs;
  const auto& descriptor = state_.font_descriptor;
  const SkFontStyle requested_style(
      descriptor.weight, SkFontStyle::kNormal_Width, descriptor.slant);
  for (const SkUnichar codepoint : DecodeUtf8(text)) {
    sk_sp<SkTypeface> typeface =
        font_manager_ ? font_manager_->MatchGlyph(
                            descriptor.families, requested_style, codepoint)
                      : nullptr;
    SkFont font = MakeFontForTypeface(std::move(typeface));
    if (runs.empty() ||
        runs.back().font.getTypeface() != font.getTypeface()) {
      runs.push_back(TextRun{std::move(font), {}});
    }
    runs.back().glyphs.push_back(
        runs.back().font.getTypeface()
            ? runs.back().font.getTypeface()->unicharToGlyph(codepoint)
            : 0);
  }
  return runs;
}

SkPaint Canvas2DContext::MakeFillPaint() const {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  if (state_.fill_source) {
    paint.setColor(SkColorSetARGB(static_cast<U8CPU>(
                                      std::lround(state_.global_alpha * 255.0f)),
                                  255, 255, 255));
    paint.setShader(state_.fill_source->shader());
  } else {
    paint.setColor(ApplyGlobalAlpha(state_.fill_style, state_.global_alpha));
  }
  ConfigurePaint(&paint, false);
  return paint;
}

SkPaint Canvas2DContext::MakeStrokePaint() const {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  if (state_.stroke_source) {
    paint.setColor(SkColorSetARGB(static_cast<U8CPU>(
                                      std::lround(state_.global_alpha * 255.0f)),
                                  255, 255, 255));
    paint.setShader(state_.stroke_source->shader());
  } else {
    paint.setColor(ApplyGlobalAlpha(state_.stroke_style, state_.global_alpha));
  }
  paint.setStrokeWidth(state_.line_width);
  ConfigurePaint(&paint, true);
  return paint;
}

SkPaint Canvas2DContext::MakeTextPaint(bool stroke) const {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(stroke ? SkPaint::kStroke_Style : SkPaint::kFill_Style);
  const auto& source = stroke ? state_.stroke_source : state_.fill_source;
  if (source) {
    paint.setColor(SkColorSetARGB(static_cast<U8CPU>(
                                      std::lround(state_.global_alpha * 255.0f)),
                                  255, 255, 255));
    paint.setShader(source->shader());
  } else {
    paint.setColor(ApplyGlobalAlpha(stroke ? state_.stroke_style : state_.fill_style,
                                    state_.global_alpha));
  }
  paint.setStrokeWidth(state_.line_width);
  ConfigurePaint(&paint, stroke);
  return paint;
}

void Canvas2DContext::ConfigurePaint(SkPaint* paint, bool allow_line_dash) const {
  paint->setBlendMode(state_.blend_mode);
  paint->setStrokeCap(state_.stroke_cap);
  paint->setStrokeJoin(state_.stroke_join);
  paint->setStrokeMiter(state_.miter_limit);
  if (allow_line_dash && !state_.line_dash.empty()) {
    std::vector<SkScalar> intervals(state_.line_dash.begin(), state_.line_dash.end());
    if (intervals.size() % 2 == 1) {
      intervals.insert(intervals.end(), state_.line_dash.begin(),
                       state_.line_dash.end());
    }
    paint->setPathEffect(
        SkDashPathEffect::Make(intervals, state_.line_dash_offset));
  } else {
    paint->setPathEffect(nullptr);
  }

  const bool has_shadow =
      (SkColorGetA(state_.shadow_color) > 0) &&
      (state_.shadow_blur > 0.0f || state_.shadow_offset_x != 0.0f ||
       state_.shadow_offset_y != 0.0f);
  if (has_shadow) {
    const float sigma = ComputeShadowSigma(state_.shadow_blur);
    paint->setImageFilter(SkImageFilters::DropShadow(
        state_.shadow_offset_x, state_.shadow_offset_y, sigma, sigma,
        ApplyGlobalAlpha(state_.shadow_color, state_.global_alpha), nullptr));
  } else {
    paint->setImageFilter(nullptr);
  }
}

void Canvas2DContext::DrawText(std::string_view text, float x, float y,
                               const SkPaint& paint) const {
  RecordDrawOp();

  const auto runs = MakeTextRuns(text);
  const TextMetrics text_metrics = MeasureTextRuns(runs);

  float draw_x = x;
  if (state_.text_align == "center") {
    draw_x -= text_metrics.width * 0.5f;
  } else if (state_.text_align == "right" || state_.text_align == "end") {
    draw_x -= text_metrics.width;
  }

  const float baseline_y = y + text_metrics.baseline_offset;

  SkTextBlobBuilder builder;
  float pen_x = 0.0f;
  for (const auto& run : runs) {
    const auto& buffer = builder.allocRun(
        run.font, static_cast<int>(run.glyphs.size()), pen_x, 0.0f);
    std::copy(run.glyphs.begin(), run.glyphs.end(), buffer.glyphs);
    for (const SkGlyphID glyph : run.glyphs) {
      pen_x += run.font.getWidth(glyph);
    }
  }
  if (auto blob = builder.make()) {
    surface_->canvas()->drawTextBlob(blob, draw_x, baseline_y, paint);
  }
}

void Canvas2DContext::RecordDrawOp() const {
  if (draw_op_callback_) {
    draw_op_callback_();
  }
}

}  // namespace canvas_engine
