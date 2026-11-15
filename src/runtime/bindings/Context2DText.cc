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

void ScriptEngineImpl::CtxMeasureText(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!handle || info.Length() < 1) {
    ThrowTypeError(info.GetIsolate(), "measureText(text) requires text");
    return;
  }

  const auto metrics =
      handle->context->MeasureText(ToStdString(info.GetIsolate(), info[0]));
  auto context = info.GetIsolate()->GetCurrentContext();
  auto object = v8::Object::New(info.GetIsolate());
  object
      ->Set(context, v8::String::NewFromUtf8Literal(info.GetIsolate(), "width"),
            v8::Number::New(info.GetIsolate(), metrics.width))
      .Check();
  object
      ->Set(context,
            v8::String::NewFromUtf8Literal(info.GetIsolate(),
                                          "actualBoundingBoxAscent"),
            v8::Number::New(info.GetIsolate(),
                            metrics.actual_bounding_box_ascent))
      .Check();
  object
      ->Set(context,
            v8::String::NewFromUtf8Literal(info.GetIsolate(),
                                          "actualBoundingBoxDescent"),
            v8::Number::New(info.GetIsolate(),
                            metrics.actual_bounding_box_descent))
      .Check();
  info.GetReturnValue().Set(object);
}

void ScriptEngineImpl::CtxFillText(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  if (!handle || info.Length() < 1 || !ExtractDouble(info, 1, &x) ||
      !ExtractDouble(info, 2, &y)) {
    ThrowTypeError(info.GetIsolate(),
                   "fillText(text, x, y, maxWidth?) requires text and coordinates");
    return;
  }

  handle->context->FillText(ToStdString(info.GetIsolate(), info[0]),
                            static_cast<float>(x), static_cast<float>(y));
}

void ScriptEngineImpl::CtxStrokeText(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double x = 0.0;
  double y = 0.0;
  if (!handle || info.Length() < 1 || !ExtractDouble(info, 1, &x) ||
      !ExtractDouble(info, 2, &y)) {
    ThrowTypeError(info.GetIsolate(),
                   "strokeText(text, x, y, maxWidth?) requires text and coordinates");
    return;
  }

  handle->context->StrokeText(ToStdString(info.GetIsolate(), info[0]),
                              static_cast<float>(x), static_cast<float>(y));
}


}  // namespace canvas_engine
