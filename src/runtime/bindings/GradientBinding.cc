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

v8::Local<v8::FunctionTemplate> ScriptEngineImpl::GetGradientTemplate() {
  if (!gradient_template_.IsEmpty()) {
    return gradient_template_.Get(isolate_);
  }

  auto tpl = v8::FunctionTemplate::New(isolate_);
  tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate_, "CanvasGradient"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  BindMethods(isolate_, tpl->PrototypeTemplate(),
              {{"addColorStop", GradientAddColorStop, 0}});
  gradient_template_.Reset(isolate_, tpl);
  return tpl;
}

void ScriptEngineImpl::CtxCreateLinearGradient(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  auto* engine = From(info.GetIsolate());
  float x0 = 0.0f;
  float y0 = 0.0f;
  float x1 = 0.0f;
  float y1 = 0.0f;
  if (!handle || !ExtractFloat(info.GetIsolate(), info[0], &x0) ||
      !ExtractFloat(info.GetIsolate(), info[1], &y0) ||
      !ExtractFloat(info.GetIsolate(), info[2], &x1) ||
      !ExtractFloat(info.GetIsolate(), info[3], &y1)) {
    ThrowTypeError(info.GetIsolate(),
                   "createLinearGradient(x0, y0, x1, y1) requires four numbers");
    return;
  }

  const std::uint64_t handle_id = engine->NextHandleId();
  auto it = engine->gradients_
                .emplace(handle_id,
                         std::make_unique<GradientHandle>(
                             handle_id,
                             std::make_shared<GradientSource>(
                                 GradientSource::Kind::kLinear, x0, y0, 0.0f,
                                 x1, y1, 0.0f)))
                .first;
  auto* gradient = it->second.get();
  engine->handle_registry_->Register(&gradient->header, gradient);

  auto context = info.GetIsolate()->GetCurrentContext();
  auto object = engine->GetGradientTemplate()
                    ->InstanceTemplate()
                    ->NewInstance(context)
                    .ToLocalChecked();
  object->SetAlignedPointerInInternalField(0, &gradient->header,
                                           kGradientHandleTag);
  gradient->object.Reset(info.GetIsolate(), object);
  gradient->finalizer_data = std::make_unique<HandleFinalizerData>(
      HandleFinalizerData{engine, HandleFinalizerData::Kind::kGradient,
                          handle_id});
  gradient->object.SetWeak(gradient->finalizer_data.get(), HandleWeakFinalizer,
                           v8::WeakCallbackType::kParameter);
  info.GetReturnValue().Set(object);
}

void ScriptEngineImpl::CtxCreateRadialGradient(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  auto* engine = From(info.GetIsolate());
  float x0 = 0.0f;
  float y0 = 0.0f;
  float r0 = 0.0f;
  float x1 = 0.0f;
  float y1 = 0.0f;
  float r1 = 0.0f;
  if (!handle || !ExtractFloat(info.GetIsolate(), info[0], &x0) ||
      !ExtractFloat(info.GetIsolate(), info[1], &y0) ||
      !ExtractFloat(info.GetIsolate(), info[2], &r0) ||
      !ExtractFloat(info.GetIsolate(), info[3], &x1) ||
      !ExtractFloat(info.GetIsolate(), info[4], &y1) ||
      !ExtractFloat(info.GetIsolate(), info[5], &r1)) {
    ThrowTypeError(
        info.GetIsolate(),
        "createRadialGradient(x0, y0, r0, x1, y1, r1) requires six numbers");
    return;
  }

  const std::uint64_t handle_id = engine->NextHandleId();
  auto it = engine->gradients_
                .emplace(handle_id,
                         std::make_unique<GradientHandle>(
                             handle_id,
                             std::make_shared<GradientSource>(
                                 GradientSource::Kind::kRadial, x0, y0, r0, x1,
                                 y1, r1)))
                .first;
  auto* gradient = it->second.get();
  engine->handle_registry_->Register(&gradient->header, gradient);

  auto context = info.GetIsolate()->GetCurrentContext();
  auto object = engine->GetGradientTemplate()
                    ->InstanceTemplate()
                    ->NewInstance(context)
                    .ToLocalChecked();
  object->SetAlignedPointerInInternalField(0, &gradient->header,
                                           kGradientHandleTag);
  gradient->object.Reset(info.GetIsolate(), object);
  gradient->finalizer_data = std::make_unique<HandleFinalizerData>(
      HandleFinalizerData{engine, HandleFinalizerData::Kind::kGradient,
                          handle_id});
  gradient->object.SetWeak(gradient->finalizer_data.get(), HandleWeakFinalizer,
                           v8::WeakCallbackType::kParameter);
  info.GetReturnValue().Set(object);
}

void ScriptEngineImpl::GradientAddColorStop(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* gradient = UnwrapHandle<GradientHandle>(
      info.GetIsolate(), info.This(), kGradientHandleTag);
  if (!gradient || info.Length() < 2) {
    ThrowTypeError(info.GetIsolate(),
                   "addColorStop(offset, color) requires offset and color");
    return;
  }

  float offset = 0.0f;
  if (!ExtractFloat(info.GetIsolate(), info[0], &offset) || !info[1]->IsString()) {
    ThrowTypeError(info.GetIsolate(),
                   "addColorStop(offset, color) requires numeric offset and CSS color");
    return;
  }

  auto parsed = ParseCssColor(ToStdString(info.GetIsolate(), info[1]));
  if (!parsed.has_value()) {
    ThrowTypeError(info.GetIsolate(), "addColorStop color must be a valid CSS color");
    return;
  }

  gradient->source->AddColorStop(std::clamp(offset, 0.0f, 1.0f),
                                 parsed.value());
}


}  // namespace canvas_engine
