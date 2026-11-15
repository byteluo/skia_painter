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

v8::Local<v8::FunctionTemplate> ScriptEngineImpl::GetImageTemplate() {
  if (!image_template_.IsEmpty()) {
    return image_template_.Get(isolate_);
  }

  auto tpl = v8::FunctionTemplate::New(isolate_, ImageConstructor);
  tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate_, "Image"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "width"), ImageWidthGetter,
      nullptr);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "height"), ImageHeightGetter,
      nullptr);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "src"), ImageSrcGetter,
      ImageSrcSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "complete"), ImageCompleteGetter,
      nullptr);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "onload"), ImageOnloadGetter,
      ImageOnloadSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "onerror"), ImageOnerrorGetter,
      ImageOnerrorSetter);

  image_template_.Reset(isolate_, tpl);
  return tpl;
}

void ScriptEngineImpl::ImageConstructor(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (!info.IsConstructCall()) {
    ThrowTypeError(isolate, "Image must be constructed with new");
    return;
  }

  auto* engine = From(isolate);
  const std::uint64_t handle_id = engine->NextHandleId();
  auto it = engine->images_
                .emplace(handle_id,
                         std::make_unique<ImageHandle>(handle_id, isolate))
                .first;
  auto* handle = it->second.get();
  engine->handle_registry_->Register(&handle->header, handle);
  handle->object.Reset(isolate, info.This());
  handle->finalizer_data = std::make_unique<HandleFinalizerData>(
      HandleFinalizerData{engine, HandleFinalizerData::Kind::kImage,
                          handle_id});
  handle->object.SetWeak(handle->finalizer_data.get(), HandleWeakFinalizer,
                         v8::WeakCallbackType::kParameter);
  info.This()->SetAlignedPointerInInternalField(0, &handle->header,
                                                kImageHandleTag);
  info.GetReturnValue().Set(info.This());
}

void ScriptEngineImpl::ImageWidthGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle =
      UnwrapHandle<ImageHandle>(info.GetIsolate(),
          V8Compat::Holder(info), kImageHandleTag);
  if (!handle || !handle->asset) {
    info.GetReturnValue().Set(0);
    return;
  }
  info.GetReturnValue().Set(handle->asset->width());
}

void ScriptEngineImpl::ImageHeightGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle =
      UnwrapHandle<ImageHandle>(info.GetIsolate(),
          V8Compat::Holder(info), kImageHandleTag);
  if (!handle || !handle->asset) {
    info.GetReturnValue().Set(0);
    return;
  }
  info.GetReturnValue().Set(handle->asset->height());
}

void ScriptEngineImpl::ImageSrcGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle =
      UnwrapHandle<ImageHandle>(info.GetIsolate(),
          V8Compat::Holder(info), kImageHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(ToV8String(info.GetIsolate(), handle->src));
}

void ScriptEngineImpl::ImageSrcSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const V8Compat::SetterInfo& info) {
  V8Compat::SetSetterSuccess(info);
  auto* handle =
      UnwrapHandle<ImageHandle>(info.GetIsolate(),
          V8Compat::Holder(info), kImageHandleTag);
  if (!handle || !value->IsString()) {
    ThrowTypeError(info.GetIsolate(), "Image.src must be a string");
    return;
  }

  auto* engine = From(info.GetIsolate());
  handle->src = ToStdString(info.GetIsolate(), value);
  const std::uint64_t generation = ++handle->source_generation;
  handle->asset.reset();
  handle->complete = false;
  handle->failed = false;

  if (StartsWithCaseInsensitive(handle->src, "data:")) {
    std::vector<std::uint8_t> encoded;
    if (DecodeDataUrl(handle->src, &encoded)) {
      handle->asset = ImageAsset::LoadFromEncodedBytes(
          std::span<const std::uint8_t>(encoded.data(), encoded.size()),
          handle->src);
    }
  } else if (StartsWithCaseInsensitive(handle->src, "http://") ||
             StartsWithCaseInsensitive(handle->src, "https://")) {
    static bool http_warning_printed = false;
    if (!http_warning_printed) {
      std::cerr << "[canvas_engine] HTTP(S) image loading is unsupported "
                   "without network access"
                << std::endl;
      http_warning_printed = true;
    }
  } else {
    const auto resolved = engine->ResolveScriptPath(handle->src);
    handle->asset = ImageAsset::LoadFromFile(resolved.string());
  }
  handle->complete = true;
  handle->failed = !handle->asset;

  const std::uint64_t image_id = handle->id;
  engine->EnqueueNativeTask(
      [engine, image_id, generation]() {
        const auto it = engine->images_.find(image_id);
        if (it == engine->images_.end()) {
          return;
        }
        auto* current = it->second.get();
        if (current->source_generation != generation) {
          return;
        }

        auto& callback_handle =
            current->failed ? current->onerror : current->onload;
        if (callback_handle.IsEmpty()) {
          return;
        }
        v8::Local<v8::Function> callback =
            callback_handle.Get(engine->isolate_);
        v8::Local<v8::Object> self = current->object.Get(engine->isolate_);
        v8::TryCatch try_catch(engine->isolate_);
        v8::Local<v8::Value> unused;
        if (!callback
                 ->Call(engine->context_.Get(engine->isolate_), self, 0, nullptr)
                 .ToLocal(&unused)) {
          engine->last_error_ =
              FormatException(engine->isolate_, &try_catch);
        }
      },
      engine->NowMs());
}

void ScriptEngineImpl::ImageCompleteGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle =
      UnwrapHandle<ImageHandle>(info.GetIsolate(),
          V8Compat::Holder(info), kImageHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(handle->complete);
}

void ScriptEngineImpl::ImageOnloadGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle =
      UnwrapHandle<ImageHandle>(info.GetIsolate(),
          V8Compat::Holder(info), kImageHandleTag);
  if (!handle || handle->onload.IsEmpty()) {
    return;
  }
  info.GetReturnValue().Set(handle->onload.Get(info.GetIsolate()));
}

void ScriptEngineImpl::ImageOnloadSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const V8Compat::SetterInfo& info) {
  V8Compat::SetSetterSuccess(info);
  auto* handle =
      UnwrapHandle<ImageHandle>(info.GetIsolate(),
          V8Compat::Holder(info), kImageHandleTag);
  if (!handle) {
    return;
  }

  handle->onload.Reset();
  if (!value->IsFunction()) {
    return;
  }

  handle->onload.Reset(info.GetIsolate(), value.As<v8::Function>());
}

void ScriptEngineImpl::ImageOnerrorGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle =
      UnwrapHandle<ImageHandle>(info.GetIsolate(),
          V8Compat::Holder(info), kImageHandleTag);
  if (!handle || handle->onerror.IsEmpty()) {
    return;
  }
  info.GetReturnValue().Set(handle->onerror.Get(info.GetIsolate()));
}

void ScriptEngineImpl::ImageOnerrorSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const V8Compat::SetterInfo& info) {
  V8Compat::SetSetterSuccess(info);
  auto* handle =
      UnwrapHandle<ImageHandle>(info.GetIsolate(),
          V8Compat::Holder(info), kImageHandleTag);
  if (!handle) {
    return;
  }

  handle->onerror.Reset();
  if (!value->IsFunction()) {
    return;
  }

  handle->onerror.Reset(info.GetIsolate(), value.As<v8::Function>());
}


}  // namespace canvas_engine
