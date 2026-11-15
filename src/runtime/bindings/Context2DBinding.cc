#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "BindingSupport.h"
#include "../EventLoop.h"
#include "../HandleRegistry.h"
#include "canvas_engine/runtime/V8Platform.h"
#include "include/effects/SkGradient.h"

namespace canvas_engine {

using namespace binding_internal;

namespace {

bool ParseFillRule(v8::Isolate* isolate, v8::Local<v8::Value> value,
                   const char* method, SkPathFillType* fill_type) {
  if (value->IsUndefined()) {
    return true;
  }
  const std::string error =
      std::string(method) + " fillRule must be nonzero or evenodd";
  if (!value->IsString()) {
    isolate->ThrowException(
        v8::Exception::TypeError(ToV8String(isolate, error)));
    return false;
  }
  const std::string fill_rule = ToStdString(isolate, value);
  if (fill_rule == "evenodd") {
    *fill_type = SkPathFillType::kEvenOdd;
    return true;
  }
  if (fill_rule == "nonzero") {
    *fill_type = SkPathFillType::kWinding;
    return true;
  }
  isolate->ThrowException(
      v8::Exception::TypeError(ToV8String(isolate, error)));
  return false;
}

}  // namespace

v8::Local<v8::FunctionTemplate> ScriptEngineImpl::GetContext2DTemplate() {
  if (!context_2d_template_.IsEmpty()) {
    return context_2d_template_.Get(isolate_);
  }

  auto tpl = v8::FunctionTemplate::New(isolate_);
  tpl->SetClassName(
      v8::String::NewFromUtf8Literal(isolate_,
                                     "CanvasRenderingContext2D"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  const std::array accessors = {
      BindAccessor<&Canvas2DContext::fill_style,
                   &Canvas2DContext::SetFillStyle>("fillStyle"),
      BindAccessor<&Canvas2DContext::stroke_style,
                   &Canvas2DContext::SetStrokeStyle>("strokeStyle"),
      BindAccessor<&Canvas2DContext::line_width,
                   &Canvas2DContext::SetLineWidth>("lineWidth"),
      BindAccessor<&Canvas2DContext::font, &Canvas2DContext::SetFont>("font"),
      BindAccessor<&Canvas2DContext::text_align,
                   &Canvas2DContext::SetTextAlign>("textAlign"),
      BindAccessor<&Canvas2DContext::text_baseline,
                   &Canvas2DContext::SetTextBaseline>("textBaseline"),
      BindAccessor<&Canvas2DContext::global_alpha,
                   &Canvas2DContext::SetGlobalAlpha>("globalAlpha"),
      BindAccessor<&Canvas2DContext::global_composite_operation,
                   &Canvas2DContext::SetGlobalCompositeOperation>(
          "globalCompositeOperation"),
      BindAccessor<&Canvas2DContext::line_cap,
                   &Canvas2DContext::SetLineCap>("lineCap"),
      BindAccessor<&Canvas2DContext::line_join,
                   &Canvas2DContext::SetLineJoin>("lineJoin"),
      BindAccessor<&Canvas2DContext::miter_limit,
                   &Canvas2DContext::SetMiterLimit>("miterLimit"),
      BindAccessor<&Canvas2DContext::shadow_blur,
                   &Canvas2DContext::SetShadowBlur>("shadowBlur"),
      BindAccessor<&Canvas2DContext::shadow_color,
                   &Canvas2DContext::SetShadowColor>("shadowColor"),
      BindAccessor<&Canvas2DContext::shadow_offset_x,
                   &Canvas2DContext::SetShadowOffsetX>("shadowOffsetX"),
      BindAccessor<&Canvas2DContext::shadow_offset_y,
                   &Canvas2DContext::SetShadowOffsetY>("shadowOffsetY"),
      BindAccessor<&Canvas2DContext::line_dash_offset,
                   &Canvas2DContext::SetLineDashOffset>("lineDashOffset"),
  };
  for (const auto& accessor : accessors) {
    tpl->InstanceTemplate()->SetNativeDataProperty(
        ToV8String(isolate_, accessor.name), accessor.getter, accessor.setter);
  }

  BindMethods(isolate_, tpl->PrototypeTemplate(), {
      {"save", CtxSave, 0},
      {"restore", CtxRestore, 0},
      {"translate", CtxTranslate, 0},
      {"scale", CtxScale, 0},
      {"rotate", CtxRotate, 0},
      {"transform", CtxTransform, 0},
      {"setTransform", CtxSetTransform, 0},
      {"resetTransform", CtxResetTransform, 0},
      {"clearRect", CtxClearRect, 0},
      {"fillRect", CtxFillRect, 0},
      {"strokeRect", CtxStrokeRect, 0},
      {"beginPath", CtxBeginPath, 0},
      {"moveTo", CtxMoveTo, 0},
      {"lineTo", CtxLineTo, 0},
      {"quadraticCurveTo", CtxQuadraticCurveTo, 0},
      {"bezierCurveTo", CtxBezierCurveTo, 0},
      {"rect", CtxRect, 0},
      {"arc", CtxArc, 0},
      {"arcTo", CtxArcTo, 0},
      {"ellipse", CtxEllipse, 0},
      {"closePath", CtxClosePath, 0},
      {"clip", CtxClip, 0},
      {"fill", CtxFill, 0},
      {"stroke", CtxStroke, 0},
      {"isPointInPath", CtxIsPointInPath, 0},
      {"drawImage", CtxDrawImage, 0},
      {"setLineDash", CtxSetLineDash, 0},
      {"getLineDash", CtxGetLineDash, 0},
      {"createLinearGradient", CtxCreateLinearGradient, 0},
      {"createRadialGradient", CtxCreateRadialGradient, 0},
      {"createPattern", CtxCreatePattern, 0},
      {"getImageData", CtxGetImageData, 0},
      {"putImageData", CtxPutImageData, 0},
      {"measureText", CtxMeasureText, 0},
      {"fillText", CtxFillText, 0},
      {"strokeText", CtxStrokeText, 0},
  });

  context_2d_template_.Reset(isolate_, tpl);
  return tpl;
}

void ScriptEngineImpl::CtxSave(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->Save();
  }
}

void ScriptEngineImpl::CtxRestore(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->Restore();
  }
}

void ScriptEngineImpl::CtxTranslate(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y)) {
    ThrowTypeError(info.GetIsolate(), "translate(x, y) requires two numbers");
    return;
  }
  handle->context->Translate(static_cast<float>(x), static_cast<float>(y));
}

void ScriptEngineImpl::CtxScale(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y)) {
    ThrowTypeError(info.GetIsolate(), "scale(x, y) requires two numbers");
    return;
  }
  handle->context->Scale(static_cast<float>(x), static_cast<float>(y));
}

void ScriptEngineImpl::CtxRotate(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double radians = 0.0;
  if (!handle || !ExtractDouble(info, 0, &radians)) {
    ThrowTypeError(info.GetIsolate(), "rotate(radians) requires a number");
    return;
  }
  handle->context->Rotate(static_cast<float>(radians));
}

void ScriptEngineImpl::CtxTransform(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  float a = 0.0f;
  float b = 0.0f;
  float c = 0.0f;
  float d = 0.0f;
  float e = 0.0f;
  float f = 0.0f;
  if (!handle || !ExtractSixNumbers(info, &a, &b, &c, &d, &e, &f)) {
    ThrowTypeError(info.GetIsolate(),
                   "transform(a, b, c, d, e, f) requires six numbers");
    return;
  }
  handle->context->Transform(a, b, c, d, e, f);
}

void ScriptEngineImpl::CtxSetTransform(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  float a = 0.0f;
  float b = 0.0f;
  float c = 0.0f;
  float d = 0.0f;
  float e = 0.0f;
  float f = 0.0f;
  if (!handle || !ExtractSixNumbers(info, &a, &b, &c, &d, &e, &f)) {
    ThrowTypeError(info.GetIsolate(),
                   "setTransform(a, b, c, d, e, f) requires six numbers");
    return;
  }
  handle->context->SetTransform(a, b, c, d, e, f);
}

void ScriptEngineImpl::CtxResetTransform(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->ResetTransform();
  }
}

void ScriptEngineImpl::CtxClearRect(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;
  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y) ||
      !ExtractDouble(info, 2, &width) || !ExtractDouble(info, 3, &height)) {
    ThrowTypeError(info.GetIsolate(),
                   "clearRect(x, y, width, height) requires four numbers");
    return;
  }
  handle->context->ClearRect(static_cast<float>(x), static_cast<float>(y),
                             static_cast<float>(width),
                             static_cast<float>(height));
}

void ScriptEngineImpl::CtxFillRect(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;
  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y) ||
      !ExtractDouble(info, 2, &width) || !ExtractDouble(info, 3, &height)) {
    ThrowTypeError(info.GetIsolate(),
                   "fillRect(x, y, width, height) requires four numbers");
    return;
  }
  handle->context->FillRect(static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(width),
                            static_cast<float>(height));
}

void ScriptEngineImpl::CtxStrokeRect(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;
  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y) ||
      !ExtractDouble(info, 2, &width) || !ExtractDouble(info, 3, &height)) {
    ThrowTypeError(info.GetIsolate(),
                   "strokeRect(x, y, width, height) requires four numbers");
    return;
  }
  handle->context->StrokeRect(static_cast<float>(x), static_cast<float>(y),
                              static_cast<float>(width),
                              static_cast<float>(height));
}

void ScriptEngineImpl::CtxBeginPath(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->BeginPath();
  }
}

void ScriptEngineImpl::CtxMoveTo(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y)) {
    ThrowTypeError(info.GetIsolate(), "moveTo(x, y) requires two numbers");
    return;
  }
  handle->context->MoveTo(static_cast<float>(x), static_cast<float>(y));
}

void ScriptEngineImpl::CtxLineTo(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y)) {
    ThrowTypeError(info.GetIsolate(), "lineTo(x, y) requires two numbers");
    return;
  }
  handle->context->LineTo(static_cast<float>(x), static_cast<float>(y));
}

void ScriptEngineImpl::CtxQuadraticCurveTo(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double cpx = 0.0;
  double cpy = 0.0;
  double x = 0.0;
  double y = 0.0;
  if (!handle || !ExtractDouble(info, 0, &cpx) || !ExtractDouble(info, 1, &cpy) ||
      !ExtractDouble(info, 2, &x) || !ExtractDouble(info, 3, &y)) {
    ThrowTypeError(
        info.GetIsolate(),
        "quadraticCurveTo(cpx, cpy, x, y) requires four numbers");
    return;
  }
  handle->context->QuadraticCurveTo(static_cast<float>(cpx),
                                    static_cast<float>(cpy),
                                    static_cast<float>(x),
                                    static_cast<float>(y));
}

void ScriptEngineImpl::CtxBezierCurveTo(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double cp1x = 0.0;
  double cp1y = 0.0;
  double cp2x = 0.0;
  double cp2y = 0.0;
  double x = 0.0;
  double y = 0.0;
  if (!handle || !ExtractDouble(info, 0, &cp1x) ||
      !ExtractDouble(info, 1, &cp1y) || !ExtractDouble(info, 2, &cp2x) ||
      !ExtractDouble(info, 3, &cp2y) || !ExtractDouble(info, 4, &x) ||
      !ExtractDouble(info, 5, &y)) {
    ThrowTypeError(
        info.GetIsolate(),
        "bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y) requires six numbers");
    return;
  }
  handle->context->BezierCurveTo(static_cast<float>(cp1x),
                                 static_cast<float>(cp1y),
                                 static_cast<float>(cp2x),
                                 static_cast<float>(cp2y),
                                 static_cast<float>(x),
                                 static_cast<float>(y));
}

void ScriptEngineImpl::CtxRect(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;
  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y) ||
      !ExtractDouble(info, 2, &width) || !ExtractDouble(info, 3, &height)) {
    ThrowTypeError(info.GetIsolate(),
                   "rect(x, y, width, height) requires four numbers");
    return;
  }
  handle->context->Rect(static_cast<float>(x), static_cast<float>(y),
                        static_cast<float>(width),
                        static_cast<float>(height));
}

void ScriptEngineImpl::CtxArc(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  double radius = 0.0;
  double start_angle = 0.0;
  double end_angle = 0.0;
  bool counter_clockwise = false;

  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y) ||
      !ExtractDouble(info, 2, &radius) ||
      !ExtractDouble(info, 3, &start_angle) ||
      !ExtractDouble(info, 4, &end_angle)) {
    ThrowTypeError(info.GetIsolate(),
                   "arc(x, y, radius, startAngle, endAngle, ccw?) requires five numbers");
    return;
  }

  if (info.Length() > 5) {
    counter_clockwise = info[5]->BooleanValue(info.GetIsolate());
  }

  handle->context->Arc(static_cast<float>(x), static_cast<float>(y),
                       static_cast<float>(radius),
                       static_cast<float>(start_angle),
                       static_cast<float>(end_angle), counter_clockwise);
}

void ScriptEngineImpl::CtxArcTo(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x1 = 0.0;
  double y1 = 0.0;
  double x2 = 0.0;
  double y2 = 0.0;
  double radius = 0.0;
  if (!handle || !ExtractDouble(info, 0, &x1) || !ExtractDouble(info, 1, &y1) ||
      !ExtractDouble(info, 2, &x2) || !ExtractDouble(info, 3, &y2) ||
      !ExtractDouble(info, 4, &radius)) {
    ThrowTypeError(info.GetIsolate(),
                   "arcTo(x1, y1, x2, y2, radius) requires five numbers");
    return;
  }
  handle->context->ArcTo(static_cast<float>(x1), static_cast<float>(y1),
                         static_cast<float>(x2), static_cast<float>(y2),
                         static_cast<float>(radius));
}

void ScriptEngineImpl::CtxEllipse(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  double radius_x = 0.0;
  double radius_y = 0.0;
  double rotation = 0.0;
  double start_angle = 0.0;
  double end_angle = 0.0;
  bool counter_clockwise = false;

  if (!handle || !ExtractDouble(info, 0, &x) || !ExtractDouble(info, 1, &y) ||
      !ExtractDouble(info, 2, &radius_x) || !ExtractDouble(info, 3, &radius_y) ||
      !ExtractDouble(info, 4, &rotation) || !ExtractDouble(info, 5, &start_angle) ||
      !ExtractDouble(info, 6, &end_angle)) {
    ThrowTypeError(
        info.GetIsolate(),
        "ellipse(x, y, radiusX, radiusY, rotation, startAngle, endAngle, ccw?) requires seven numbers");
    return;
  }

  if (info.Length() > 7) {
    counter_clockwise = info[7]->BooleanValue(info.GetIsolate());
  }

  handle->context->Ellipse(static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(radius_x),
                           static_cast<float>(radius_y),
                           static_cast<float>(rotation),
                           static_cast<float>(start_angle),
                           static_cast<float>(end_angle), counter_clockwise);
}

void ScriptEngineImpl::CtxClosePath(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->ClosePath();
  }
}

void ScriptEngineImpl::CtxClip(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!handle) {
    return;
  }

  int fill_rule_index = 0;
  SkPath path;
  bool has_path = false;
  if (info.Length() > 0 && info[0]->IsObject()) {
    auto* path_handle = UnwrapHandle<Path2DHandle>(
        info.GetIsolate(), info[0].As<v8::Object>(), kPath2DHandleTag);
    if (!path_handle) {
      ThrowTypeError(info.GetIsolate(), "clip(path) requires a Path2D");
      return;
    }
    path = path_handle->path.snapshot();
    has_path = true;
    fill_rule_index = 1;
  }

  SkPathFillType fill_type = SkPathFillType::kWinding;
  if (info.Length() > fill_rule_index &&
      !ParseFillRule(info.GetIsolate(), info[fill_rule_index], "clip",
                     &fill_type)) {
    return;
  }
  if (has_path) {
    handle->context->Clip(path, fill_type);
  } else {
    handle->context->Clip(fill_type);
  }
}

void ScriptEngineImpl::CtxFill(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!handle) {
    return;
  }

  int fill_rule_index = 0;
  SkPath path;
  bool has_path = false;
  if (info.Length() > 0 && info[0]->IsObject()) {
    auto* path_handle = UnwrapHandle<Path2DHandle>(
        info.GetIsolate(), info[0].As<v8::Object>(), kPath2DHandleTag);
    if (!path_handle) {
      ThrowTypeError(info.GetIsolate(), "fill(path) requires a Path2D");
      return;
    }
    path = path_handle->path.snapshot();
    has_path = true;
    fill_rule_index = 1;
  }

  SkPathFillType fill_type = SkPathFillType::kWinding;
  if (info.Length() > fill_rule_index &&
      !ParseFillRule(info.GetIsolate(), info[fill_rule_index], "fill",
                     &fill_type)) {
    return;
  }
  if (has_path) {
    handle->context->Fill(path, fill_type);
  } else {
    handle->context->Fill(fill_type);
  }
}

void ScriptEngineImpl::CtxStroke(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!handle) {
    return;
  }
  if (info.Length() > 0 && info[0]->IsObject()) {
    if (auto* path_handle = UnwrapHandle<Path2DHandle>(
            info.GetIsolate(), info[0].As<v8::Object>(), kPath2DHandleTag)) {
      handle->context->Stroke(path_handle->path.snapshot());
      return;
    }
  }
  handle->context->Stroke();
}

void ScriptEngineImpl::CtxIsPointInPath(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* context_handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!context_handle) {
    info.GetReturnValue().Set(false);
    return;
  }

  int x_index = 0;
  int y_index = 1;
  int fill_rule_index = 2;
  SkPath path;
  bool has_path = false;
  if (info.Length() > 0 && info[0]->IsObject()) {
    auto* path_handle = UnwrapHandle<Path2DHandle>(
        info.GetIsolate(), info[0].As<v8::Object>(), kPath2DHandleTag);
    if (!path_handle) {
      ThrowTypeError(info.GetIsolate(), "isPointInPath(path, x, y) requires a Path2D");
      return;
    }
    path = path_handle->path.snapshot();
    has_path = true;
    x_index = 1;
    y_index = 2;
    fill_rule_index = 3;
  }

  double x = 0.0;
  double y = 0.0;
  if (!ExtractDouble(info, x_index, &x) ||
      !ExtractDouble(info, y_index, &y)) {
    ThrowTypeError(info.GetIsolate(), "isPointInPath requires numeric coordinates");
    return;
  }

  SkPathFillType fill_type = SkPathFillType::kWinding;
  if (info.Length() > fill_rule_index &&
      !ParseFillRule(info.GetIsolate(), info[fill_rule_index], "isPointInPath",
                     &fill_type)) {
    return;
  }

  const bool contains =
      has_path ? context_handle->context->IsPointInPath(
                     path, static_cast<float>(x), static_cast<float>(y),
                     fill_type)
               : context_handle->context->IsPointInPath(
                     static_cast<float>(x), static_cast<float>(y), fill_type);
  info.GetReturnValue().Set(contains);
}


}  // namespace canvas_engine
