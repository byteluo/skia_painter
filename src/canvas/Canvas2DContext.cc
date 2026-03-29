#include "canvas_engine/canvas/Canvas2DContext.h"

#include <cmath>

#include "include/core/SkBlendMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkScalar.h"

#include "canvas_engine/canvas/ColorParser.h"

namespace canvas_engine {

namespace {

constexpr float kRadiansToDegrees = 57.29577951308232f;

}  // namespace

Canvas2DContext::Canvas2DContext(std::shared_ptr<CanvasSurface> surface)
    : surface_(std::move(surface)) {}

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

SkPaint Canvas2DContext::MakeFillPaint() const {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(state_.fill_style);
  return paint;
}

SkPaint Canvas2DContext::MakeStrokePaint() const {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(state_.stroke_style);
  paint.setStrokeWidth(state_.line_width);
  return paint;
}

}  // namespace canvas_engine
