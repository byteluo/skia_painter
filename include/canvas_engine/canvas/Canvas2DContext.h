#pragma once

#include <cstdint>
#include <optional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "include/core/SkBlendMode.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkShader.h"

#include "canvas_engine/canvas/CanvasSurface.h"

namespace canvas_engine {

class Canvas2DContext {
 public:
  struct ImageData {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> data;
  };

  struct TextMetrics {
    float width = 0.0f;
    float actual_bounding_box_ascent = 0.0f;
    float actual_bounding_box_descent = 0.0f;
  };

  explicit Canvas2DContext(std::shared_ptr<CanvasSurface> surface);

  bool SetFillStyle(std::string_view css_color);
  bool SetStrokeStyle(std::string_view css_color);
  bool SetLineWidth(float value);
  bool SetFont(std::string_view font);
  bool SetTextAlign(std::string_view value);
  bool SetTextBaseline(std::string_view value);
  bool SetGlobalAlpha(float value);
  bool SetGlobalCompositeOperation(std::string_view value);
  bool SetLineCap(std::string_view value);
  bool SetLineJoin(std::string_view value);
  bool SetMiterLimit(float value);
  bool SetShadowBlur(float value);
  bool SetShadowColor(std::string_view css_color);
  void SetShadowOffsetX(float value);
  void SetShadowOffsetY(float value);
  void SetFillShader(sk_sp<SkShader> shader, std::string_view description);
  void SetStrokeShader(sk_sp<SkShader> shader, std::string_view description);
  void SetLineDash(const std::vector<float>& segments);
  const std::vector<float>& line_dash() const { return state_.line_dash; }
  void SetLineDashOffset(float value);
  float line_dash_offset() const { return state_.line_dash_offset; }

  const std::string& fill_style() const { return state_.fill_style_css; }
  const std::string& stroke_style() const { return state_.stroke_style_css; }
  float line_width() const { return state_.line_width; }
  const std::string& font() const { return state_.font_css; }
  const std::string& text_align() const { return state_.text_align; }
  const std::string& text_baseline() const { return state_.text_baseline; }
  float global_alpha() const { return state_.global_alpha; }
  const std::string& global_composite_operation() const {
    return state_.global_composite_operation;
  }
  const std::string& line_cap() const { return state_.line_cap; }
  const std::string& line_join() const { return state_.line_join; }
  float miter_limit() const { return state_.miter_limit; }
  float shadow_blur() const { return state_.shadow_blur; }
  const std::string& shadow_color() const { return state_.shadow_color_css; }
  float shadow_offset_x() const { return state_.shadow_offset_x; }
  float shadow_offset_y() const { return state_.shadow_offset_y; }

  void ResetState();

  void Save();
  void Restore();

  void Translate(float dx, float dy);
  void Scale(float sx, float sy);
  void Rotate(float radians);
  void Transform(float a, float b, float c, float d, float e, float f);
  void SetTransform(float a, float b, float c, float d, float e, float f);
  void ResetTransform();

  void ClearRect(float x, float y, float width, float height);
  void FillRect(float x, float y, float width, float height);
  void StrokeRect(float x, float y, float width, float height);

  void BeginPath();
  void MoveTo(float x, float y);
  void LineTo(float x, float y);
  void QuadraticCurveTo(float cpx, float cpy, float x, float y);
  void BezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x,
                     float y);
  void Rect(float x, float y, float width, float height);
  void Arc(float x, float y, float radius, float start_angle, float end_angle,
           bool counter_clockwise);
  void ArcTo(float x1, float y1, float x2, float y2, float radius);
  void Ellipse(float x, float y, float radius_x, float radius_y, float rotation,
               float start_angle, float end_angle, bool counter_clockwise);
  void ClosePath();
  void Clip();
  void Fill();
  void Stroke();
  void DrawImage(const sk_sp<SkImage>& image, float sx, float sy, float sw,
                 float sh, float dx, float dy, float dw, float dh);
  std::optional<ImageData> GetImageData(int x, int y, int width, int height) const;
  bool PutImageData(const ImageData& image_data, int dx, int dy);
  TextMetrics MeasureText(std::string_view text) const;
  void FillText(std::string_view text, float x, float y);
  void StrokeText(std::string_view text, float x, float y);

 private:
  struct State {
    SkColor fill_style = SK_ColorBLACK;
    SkColor stroke_style = SK_ColorBLACK;
    std::string fill_style_css = "#000000";
    std::string stroke_style_css = "#000000";
    float line_width = 1.0f;
    std::string font_css = "10px sans-serif";
    float font_size = 10.0f;
    std::string text_align = "start";
    std::string text_baseline = "alphabetic";
    float global_alpha = 1.0f;
    SkBlendMode blend_mode = SkBlendMode::kSrcOver;
    std::string global_composite_operation = "source-over";
    SkPaint::Cap stroke_cap = SkPaint::kButt_Cap;
    std::string line_cap = "butt";
    SkPaint::Join stroke_join = SkPaint::kMiter_Join;
    std::string line_join = "miter";
    float miter_limit = 10.0f;
    float shadow_blur = 0.0f;
    SkColor shadow_color = SK_ColorTRANSPARENT;
    std::string shadow_color_css = "rgba(0, 0, 0, 0)";
    float shadow_offset_x = 0.0f;
    float shadow_offset_y = 0.0f;
    sk_sp<SkShader> fill_shader;
    sk_sp<SkShader> stroke_shader;
    std::vector<float> line_dash;
    float line_dash_offset = 0.0f;
  };

  static State MakeDefaultState();

  void ApplyDeviceScale();
  SkFont MakeFont() const;
  SkPaint MakeFillPaint() const;
  SkPaint MakeStrokePaint() const;
  SkPaint MakeTextPaint(bool stroke) const;
  void ConfigurePaint(SkPaint* paint, bool allow_line_dash) const;
  void DrawText(std::string_view text, float x, float y, const SkPaint& paint) const;

  std::shared_ptr<CanvasSurface> surface_;
  State state_ = MakeDefaultState();
  std::vector<State> state_stack_;
  SkPathBuilder current_path_;
};

}  // namespace canvas_engine
