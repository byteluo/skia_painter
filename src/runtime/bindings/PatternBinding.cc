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

v8::Local<v8::FunctionTemplate> ScriptEngineImpl::GetPatternTemplate() {
  if (!pattern_template_.IsEmpty()) {
    return pattern_template_.Get(isolate_);
  }

  auto tpl = v8::FunctionTemplate::New(isolate_);
  tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate_, "CanvasPattern"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  pattern_template_.Reset(isolate_, tpl);
  return tpl;
}

void ScriptEngineImpl::CtxCreatePattern(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  auto* engine = From(info.GetIsolate());
  if (!handle || info.Length() < 1 || !info[0]->IsObject()) {
    ThrowTypeError(info.GetIsolate(),
                   "createPattern(image, repeat?) requires an Image or Canvas source");
    return;
  }

  std::string repeat = "repeat";
  if (info.Length() > 1 && !info[1]->IsUndefined() && !info[1]->IsNull()) {
    if (!info[1]->IsString()) {
      ThrowTypeError(info.GetIsolate(), "createPattern repeat must be a string");
      return;
    }
    repeat = ToStdString(info.GetIsolate(), info[1]);
    if (repeat != "repeat" && repeat != "repeat-x" && repeat != "repeat-y" &&
        repeat != "no-repeat") {
      ThrowTypeError(info.GetIsolate(), "createPattern repeat must be a valid repetition mode");
      return;
    }
  }

  auto source = info[0].As<v8::Object>();
  sk_sp<SkImage> image;
  float source_pixel_ratio = 1.0f;

  if (auto* image_handle = UnwrapHandle<ImageHandle>(
          info.GetIsolate(), source, kImageHandleTag)) {
    if (image_handle->asset) {
      image = image_handle->asset->image();
    }
  } else if (auto* canvas_handle = UnwrapHandle<CanvasHandle>(
                 info.GetIsolate(), source, kCanvasHandleTag)) {
    image = canvas_handle->surface->MakeImageSnapshot();
    source_pixel_ratio = canvas_handle->surface->pixel_ratio();
  }

  if (!image) {
    info.GetReturnValue().Set(v8::Null(info.GetIsolate()));
    return;
  }

  const std::uint64_t handle_id = engine->NextHandleId();
  auto it = engine->patterns_
                .emplace(handle_id, std::make_unique<PatternHandle>(handle_id))
                .first;
  auto* pattern = it->second.get();

  SkMatrix local_matrix = SkMatrix::I();
  if (source_pixel_ratio != 1.0f) {
    local_matrix.setScale(1.0f / source_pixel_ratio, 1.0f / source_pixel_ratio);
  }

  auto shader = image->makeShader(
      RepeatModeToTileMode(repeat, true), RepeatModeToTileMode(repeat, false),
      SkSamplingOptions(SkFilterMode::kLinear), &local_matrix);
  if (!shader) {
    engine->patterns_.erase(handle_id);
    info.GetReturnValue().Set(v8::Null(info.GetIsolate()));
    return;
  }
  pattern->source = std::make_shared<FixedShaderSource>(
      std::move(shader), "[object CanvasPattern]");
  engine->handle_registry_->Register(&pattern->header, pattern);

  auto context = info.GetIsolate()->GetCurrentContext();
  auto object = engine->GetPatternTemplate()
                    ->InstanceTemplate()
                    ->NewInstance(context)
                    .ToLocalChecked();
  object->SetAlignedPointerInInternalField(0, &pattern->header,
                                           kPatternHandleTag);
  pattern->object.Reset(info.GetIsolate(), object);
  pattern->finalizer_data = std::make_unique<HandleFinalizerData>(
      HandleFinalizerData{engine, HandleFinalizerData::Kind::kPattern,
                          handle_id});
  pattern->object.SetWeak(pattern->finalizer_data.get(), HandleWeakFinalizer,
                          v8::WeakCallbackType::kParameter);
  info.GetReturnValue().Set(object);
}


}  // namespace canvas_engine
