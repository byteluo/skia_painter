#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

#include "BindingSupport.h"
#include "../HandleRegistry.h"
#include "include/utils/SkParsePath.h"

namespace canvas_engine {

using namespace binding_internal;

namespace {

constexpr float kPathRadiansToDegrees = 180.0f / 3.14159265358979323846f;
constexpr float kPathFullCircleDegrees = 360.0f;
constexpr float kPathFullCircleEpsilon = 1.0e-4f;

bool ExtractPathNumbers(const v8::FunctionCallbackInfo<v8::Value>& info,
                        int count, float* values) {
  for (int index = 0; index < count; ++index) {
    double value = 0.0;
    if (!ExtractDouble(info, index, &value)) {
      return false;
    }
    values[index] = static_cast<float>(value);
  }
  return true;
}

}  // namespace

v8::Local<v8::FunctionTemplate> ScriptEngineImpl::GetPath2DTemplate() {
  if (!path_2d_template_.IsEmpty()) {
    return path_2d_template_.Get(isolate_);
  }

  auto tpl = v8::FunctionTemplate::New(isolate_, Path2DConstructor);
  tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate_, "Path2D"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  BindMethods(isolate_, tpl->PrototypeTemplate(), {
      {"moveTo", Path2DMoveTo, 2},
      {"lineTo", Path2DLineTo, 2},
      {"quadraticCurveTo", Path2DQuadraticCurveTo, 4},
      {"bezierCurveTo", Path2DBezierCurveTo, 6},
      {"rect", Path2DRect, 4},
      {"arc", Path2DArc, 5},
      {"arcTo", Path2DArcTo, 5},
      {"ellipse", Path2DEllipse, 7},
      {"closePath", Path2DClosePath, 0},
      {"addPath", Path2DAddPath, 1},
  });
  path_2d_template_.Reset(isolate_, tpl);
  return tpl;
}

void ScriptEngineImpl::Path2DConstructor(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (!info.IsConstructCall()) {
    ThrowTypeError(isolate, "Path2D must be constructed with new");
    return;
  }

  SkPathBuilder path;
  if (info.Length() > 0 && !info[0]->IsUndefined()) {
    if (info[0]->IsString()) {
      const std::string svg_path = ToStdString(isolate, info[0]);
      auto parsed_path = SkParsePath::FromSVGString(svg_path.c_str());
      if (!parsed_path) {
        ThrowTypeError(isolate, "Path2D requires a valid SVG path string");
        return;
      }
      path = *parsed_path;
    } else if (info[0]->IsObject()) {
      auto* source = UnwrapHandle<Path2DHandle>(
          isolate, info[0].As<v8::Object>(), kPath2DHandleTag);
      if (!source) {
        ThrowTypeError(isolate, "Path2D requires a Path2D or SVG path string");
        return;
      }
      path = source->path;
    } else {
      ThrowTypeError(isolate, "Path2D requires a Path2D or SVG path string");
      return;
    }
  }

  auto* engine = From(isolate);
  const std::uint64_t handle_id = engine->NextHandleId();
  auto it = engine->path2ds_
                .emplace(handle_id,
                         std::make_unique<Path2DHandle>(handle_id,
                                                        std::move(path)))
                .first;
  auto* handle = it->second.get();
  engine->handle_registry_->Register(&handle->header, handle);
  handle->object.Reset(isolate, info.This());
  handle->finalizer_data = std::make_unique<HandleFinalizerData>(
      HandleFinalizerData{engine, HandleFinalizerData::Kind::kPath2D,
                          handle_id});
  handle->object.SetWeak(handle->finalizer_data.get(), HandleWeakFinalizer,
                         v8::WeakCallbackType::kParameter);
  info.This()->SetAlignedPointerInInternalField(0, &handle->header,
                                                kPath2DHandleTag);
  info.GetReturnValue().Set(info.This());
}

void ScriptEngineImpl::Path2DMoveTo(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle = UnwrapHandle<Path2DHandle>(
      info.GetIsolate(), info.This(), kPath2DHandleTag);
  float values[2];
  if (!handle || !ExtractPathNumbers(info, 2, values)) {
    ThrowTypeError(info.GetIsolate(), "moveTo(x, y) requires two numbers");
    return;
  }
  handle->path.moveTo(values[0], values[1]);
}

void ScriptEngineImpl::Path2DLineTo(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle = UnwrapHandle<Path2DHandle>(
      info.GetIsolate(), info.This(), kPath2DHandleTag);
  float values[2];
  if (!handle || !ExtractPathNumbers(info, 2, values)) {
    ThrowTypeError(info.GetIsolate(), "lineTo(x, y) requires two numbers");
    return;
  }
  handle->path.lineTo(values[0], values[1]);
}

void ScriptEngineImpl::Path2DQuadraticCurveTo(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle = UnwrapHandle<Path2DHandle>(
      info.GetIsolate(), info.This(), kPath2DHandleTag);
  float values[4];
  if (!handle || !ExtractPathNumbers(info, 4, values)) {
    ThrowTypeError(info.GetIsolate(),
                   "quadraticCurveTo requires four numeric coordinates");
    return;
  }
  handle->path.quadTo(values[0], values[1], values[2], values[3]);
}

void ScriptEngineImpl::Path2DBezierCurveTo(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle = UnwrapHandle<Path2DHandle>(
      info.GetIsolate(), info.This(), kPath2DHandleTag);
  float values[6];
  if (!handle || !ExtractPathNumbers(info, 6, values)) {
    ThrowTypeError(info.GetIsolate(),
                   "bezierCurveTo requires six numeric coordinates");
    return;
  }
  handle->path.cubicTo(values[0], values[1], values[2], values[3], values[4],
                       values[5]);
}

void ScriptEngineImpl::Path2DRect(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle = UnwrapHandle<Path2DHandle>(
      info.GetIsolate(), info.This(), kPath2DHandleTag);
  float values[4];
  if (!handle || !ExtractPathNumbers(info, 4, values)) {
    ThrowTypeError(info.GetIsolate(), "rect requires four numeric coordinates");
    return;
  }
  handle->path.addRect(
      SkRect::MakeXYWH(values[0], values[1], values[2], values[3]));
}

void ScriptEngineImpl::Path2DArc(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle = UnwrapHandle<Path2DHandle>(
      info.GetIsolate(), info.This(), kPath2DHandleTag);
  float values[5];
  if (!handle || !ExtractPathNumbers(info, 5, values)) {
    ThrowTypeError(info.GetIsolate(), "arc requires five numeric coordinates");
    return;
  }
  if (values[2] < 0.0f) {
    ThrowTypeError(info.GetIsolate(), "arc radius must be non-negative");
    return;
  }
  const bool counter_clockwise =
      info.Length() > 5 && info[5]->BooleanValue(info.GetIsolate());
  float sweep = (values[4] - values[3]) * kPathRadiansToDegrees;
  if (!counter_clockwise && sweep < 0.0f) {
    sweep += kPathFullCircleDegrees;
  } else if (counter_clockwise && sweep > 0.0f) {
    sweep -= kPathFullCircleDegrees;
  }
  const SkRect oval = SkRect::MakeLTRB(
      values[0] - values[2], values[1] - values[2], values[0] + values[2],
      values[1] + values[2]);
  const float start_degrees = values[3] * kPathRadiansToDegrees;
  if (std::fabs(sweep) >= kPathFullCircleDegrees - kPathFullCircleEpsilon) {
    const float half_sweep = counter_clockwise ? -180.0f : 180.0f;
    handle->path.arcTo(oval, start_degrees, half_sweep, false);
    handle->path.arcTo(oval, start_degrees + half_sweep, half_sweep, false);
    return;
  }
  handle->path.arcTo(oval, start_degrees, sweep, false);
}

void ScriptEngineImpl::Path2DArcTo(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle = UnwrapHandle<Path2DHandle>(
      info.GetIsolate(), info.This(), kPath2DHandleTag);
  float values[5];
  if (!handle || !ExtractPathNumbers(info, 5, values)) {
    ThrowTypeError(info.GetIsolate(), "arcTo requires five numeric coordinates");
    return;
  }
  if (values[4] < 0.0f) {
    ThrowTypeError(info.GetIsolate(), "arcTo radius must be non-negative");
    return;
  }
  handle->path.arcTo(SkPoint::Make(values[0], values[1]),
                     SkPoint::Make(values[2], values[3]), values[4]);
}

void ScriptEngineImpl::Path2DEllipse(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle = UnwrapHandle<Path2DHandle>(
      info.GetIsolate(), info.This(), kPath2DHandleTag);
  float values[7];
  if (!handle || !ExtractPathNumbers(info, 7, values)) {
    ThrowTypeError(info.GetIsolate(),
                   "ellipse requires seven numeric coordinates");
    return;
  }
  if (values[2] < 0.0f || values[3] < 0.0f) {
    ThrowTypeError(info.GetIsolate(), "ellipse radii must be non-negative");
    return;
  }
  if (values[2] == 0.0f || values[3] == 0.0f) {
    return;
  }
  const bool counter_clockwise =
      info.Length() > 7 && info[7]->BooleanValue(info.GetIsolate());
  float sweep = (values[6] - values[5]) * kPathRadiansToDegrees;
  if (!counter_clockwise && sweep < 0.0f) {
    sweep += kPathFullCircleDegrees;
  } else if (counter_clockwise && sweep > 0.0f) {
    sweep -= kPathFullCircleDegrees;
  }

  SkPathBuilder ellipse_path;
  if (std::fabs(sweep) >= kPathFullCircleDegrees - kPathFullCircleEpsilon) {
    ellipse_path.addOval(SkRect::MakeLTRB(-1.0f, -1.0f, 1.0f, 1.0f),
                         counter_clockwise ? SkPathDirection::kCCW
                                           : SkPathDirection::kCW);
  } else {
    ellipse_path.arcTo(SkRect::MakeLTRB(-1.0f, -1.0f, 1.0f, 1.0f),
                       values[5] * kPathRadiansToDegrees, sweep, false);
  }

  const float sin_rotation = std::sin(values[4]);
  const float cos_rotation = std::cos(values[4]);
  const SkMatrix transform = SkMatrix::MakeAll(
      cos_rotation * values[2], -sin_rotation * values[3], values[0],
      sin_rotation * values[2], cos_rotation * values[3], values[1], 0.0f,
      0.0f, 1.0f);
  handle->path.addPath(ellipse_path.snapshot(), transform);
}

void ScriptEngineImpl::Path2DClosePath(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle = UnwrapHandle<Path2DHandle>(
          info.GetIsolate(), info.This(), kPath2DHandleTag)) {
    handle->path.close();
  }
}

void ScriptEngineImpl::Path2DAddPath(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle = UnwrapHandle<Path2DHandle>(
      info.GetIsolate(), info.This(), kPath2DHandleTag);
  if (!handle || info.Length() < 1 || !info[0]->IsObject()) {
    ThrowTypeError(info.GetIsolate(), "addPath(path) requires a Path2D");
    return;
  }
  auto* source = UnwrapHandle<Path2DHandle>(
      info.GetIsolate(), info[0].As<v8::Object>(), kPath2DHandleTag);
  if (!source) {
    ThrowTypeError(info.GetIsolate(), "addPath(path) requires a Path2D");
    return;
  }
  handle->path.addPath(source->path.snapshot());
}

}  // namespace canvas_engine
