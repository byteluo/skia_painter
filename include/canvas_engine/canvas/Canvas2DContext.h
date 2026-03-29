#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"

#include "canvas_engine/canvas/CanvasSurface.h"

namespace canvas_engine {

class Canvas2DContext {
 public:
  explicit Canvas2DContext(std::shared_ptr<CanvasSurface> surface);

  bool SetFillStyle(std::string_view css_color);
  bool SetStrokeStyle(std::string_view css_color);
  bool SetLineWidth(float value);

  const std::string& fill_style() const { return state_.fill_style_css; }
  const std::string& stroke_style() const { return state_.stroke_style_css; }
  float line_width() const { return state_.line_width; }

  void Save();
  void Restore();

  void Translate(float dx, float dy);
  void Scale(float sx, float sy);
  void Rotate(float radians);

  void ClearRect(float x, float y, float width, float height);
  void FillRect(float x, float y, float width, float height);
  void StrokeRect(float x, float y, float width, float height);

  void BeginPath();
  void MoveTo(float x, float y);
  void LineTo(float x, float y);
  void Arc(float x, float y, float radius, float start_angle, float end_angle,
           bool counter_clockwise);
  void ClosePath();
  void Fill();
  void Stroke();

 private:
  struct State {
    SkColor fill_style = SK_ColorBLACK;
    SkColor stroke_style = SK_ColorBLACK;
    std::string fill_style_css = "#000000";
    std::string stroke_style_css = "#000000";
    float line_width = 1.0f;
  };

  SkPaint MakeFillPaint() const;
  SkPaint MakeStrokePaint() const;

  std::shared_ptr<CanvasSurface> surface_;
  State state_;
  std::vector<State> state_stack_;
  SkPathBuilder current_path_;
};

}  // namespace canvas_engine
