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

v8::Local<v8::FunctionTemplate> ScriptEngineImpl::GetCanvasTemplate() {
  if (!canvas_template_.IsEmpty()) {
    return canvas_template_.Get(isolate_);
  }

  auto tpl = v8::FunctionTemplate::New(isolate_, CanvasConstructor);
  tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate_, "Canvas"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "width"), CanvasWidthGetter,
      CanvasWidthSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "height"), CanvasHeightGetter,
      CanvasHeightSetter);
  BindMethods(isolate_, tpl->PrototypeTemplate(), {
      {"getContext", CanvasGetContext, 0},
      {"saveToPng", CanvasSaveToPng, 0},
      {"setAttribute", CanvasSetAttribute, 0},
      {"addEventListener", CanvasAddEventListener, 0},
      {"removeEventListener", CanvasRemoveEventListener, 0},
  });

  canvas_template_.Reset(isolate_, tpl);
  return tpl;
}

void ScriptEngineImpl::CanvasConstructor(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (!info.IsConstructCall()) {
    ThrowTypeError(isolate, "Canvas must be constructed with new");
    return;
  }

  double width = 0.0;
  double height = 0.0;
  if (!ExtractDouble(info, 0, &width) || !ExtractDouble(info, 1, &height)) {
    ThrowTypeError(isolate, "Canvas(width, height) requires numeric arguments");
    return;
  }

  auto* engine = From(isolate);
  float pixel_ratio = engine->options_.device_pixel_ratio;
  if (info.Length() > 2 && !info[2]->IsUndefined()) {
    double requested_pixel_ratio = 0.0;
    if (!ExtractDouble(info, 2, &requested_pixel_ratio) ||
        !(requested_pixel_ratio > 0.0) ||
        !std::isfinite(requested_pixel_ratio)) {
      ThrowTypeError(isolate, "Canvas device pixel ratio must be positive");
      return;
    }
    pixel_ratio = static_cast<float>(requested_pixel_ratio);
  }
  const float exposed_pixel_ratio = GetExposedDevicePixelRatio();
  auto surface = CanvasSurface::CreateRaster(static_cast<int>(width),
                                             static_cast<int>(height),
                                             pixel_ratio);
  if (!surface) {
    ThrowError(isolate, "failed to create raster canvas surface");
    return;
  }

  const std::uint64_t handle_id = engine->NextHandleId();
  auto it = engine->canvases_
                .emplace(handle_id,
                         std::make_unique<CanvasHandle>(handle_id, surface))
                .first;
  auto* handle = it->second.get();
  engine->handle_registry_->Register(&handle->header, handle);
  handle->object.Reset(isolate, info.This());
  handle->finalizer_data = std::make_unique<HandleFinalizerData>(
      HandleFinalizerData{engine, HandleFinalizerData::Kind::kCanvas,
                          handle_id});
  handle->object.SetWeak(handle->finalizer_data.get(), HandleWeakFinalizer,
                         v8::WeakCallbackType::kParameter);
  info.This()->SetAlignedPointerInInternalField(0, &handle->header,
                                                kCanvasHandleTag);

  auto context = isolate->GetCurrentContext();
  info.This()
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "style"),
            v8::Object::New(isolate))
      .Check();
  info.This()
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "nodeName"),
            v8::String::NewFromUtf8Literal(isolate, "CANVAS"))
      .Check();
  info.This()
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "devicePixelRatio"),
            v8::Number::New(isolate, exposed_pixel_ratio))
      .Check();
  info.This()
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "dpr"),
            v8::Number::New(isolate, exposed_pixel_ratio))
      .Check();

  info.GetReturnValue().Set(info.This());
}

void ScriptEngineImpl::CanvasGetContext(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  auto* canvas = UnwrapHandle<CanvasHandle>(isolate, info.This(), kCanvasHandleTag);
  if (!canvas) {
    ThrowTypeError(isolate, "invalid Canvas instance");
    return;
  }

  if (info.Length() < 1 || !info[0]->IsString()) {
    ThrowTypeError(isolate, "getContext(type) requires a string argument");
    return;
  }

  if (ToStdString(isolate, info[0]) != "2d") {
    info.GetReturnValue().Set(v8::Null(isolate));
    return;
  }

  auto* engine = From(isolate);
  auto context = isolate->GetCurrentContext();
  if (!canvas->context_handle) {
    canvas->context_handle = std::make_unique<ContextHandle>(
        std::make_unique<Canvas2DContext>(
            canvas->surface,
            [engine]() {
              if (engine) {
                engine->RecordDrawOp();
              }
            },
            engine ? engine->font_manager_ : nullptr));
  }

  if (canvas->context_object.IsEmpty()) {
    auto object = engine->GetContext2DTemplate()
                      ->InstanceTemplate()
                      ->NewInstance(context)
                      .ToLocalChecked();
    object->SetAlignedPointerInInternalField(0, canvas->context_handle.get(),
                                             kContextHandleTag);
    object
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "__canvasEngineCanvas"),
              info.This())
        .Check();
    canvas->context_object.Reset(isolate, object);
    canvas->context_object.SetWeak();
  }

  auto context_object = canvas->context_object.Get(isolate);
  context_object
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "canvas"),
            info.This())
      .Check();
  info.GetReturnValue().Set(context_object);
}

void ScriptEngineImpl::CanvasSaveToPng(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  auto* canvas = UnwrapHandle<CanvasHandle>(isolate, info.This(), kCanvasHandleTag);
  if (!canvas) {
    ThrowTypeError(isolate, "invalid Canvas instance");
    return;
  }

  if (info.Length() < 1 || !info[0]->IsString()) {
    ThrowTypeError(isolate, "saveToPng(path) requires a string path");
    return;
  }

  bool drain_pending_tasks = true;
  PngCompression png_compression = PngCompression::kDefault;
  const auto parse_png_compression = [&](v8::Local<v8::Value> value) {
    if (!value->IsString()) {
      ThrowTypeError(isolate,
                     "saveToPng PNG compression must be a string");
      return false;
    }
    const std::string compression = ToStdString(isolate, value);
    if (compression == "fast") {
      png_compression = PngCompression::kFast;
    } else if (compression == "default") {
      png_compression = PngCompression::kDefault;
    } else if (compression == "small") {
      png_compression = PngCompression::kSmall;
    } else {
      ThrowTypeError(isolate,
                     "saveToPng PNG compression must be fast, default, or "
                     "small");
      return false;
    }
    return true;
  };
  if (info.Length() > 1) {
    if (info[1]->IsBoolean()) {
      drain_pending_tasks = info[1]->BooleanValue(isolate);
    } else if (info[1]->IsString()) {
      if (!parse_png_compression(info[1])) {
        return;
      }
    } else {
      ThrowTypeError(isolate,
                     "saveToPng second argument must be a boolean or "
                     "compression string");
      return;
    }
  }
  if (info.Length() > 2) {
    if (!info[1]->IsBoolean() || !parse_png_compression(info[2])) {
      if (!info[1]->IsBoolean()) {
        ThrowTypeError(
            isolate,
            "saveToPng third argument requires a boolean second argument");
      }
      return;
    }
  }

  auto* engine = From(isolate);
  const std::string output_path = ToStdString(isolate, info[0]);
  if (engine && drain_pending_tasks && !engine->DrainPendingTasks()) {
    ThrowError(isolate, engine->last_error_);
    return;
  }
  const auto resolved = engine->ResolveScriptPath(output_path);
  if (!canvas->surface->SavePng(resolved.string(), png_compression)) {
    ThrowError(isolate, "failed to write png: " + resolved.string());
  }
}

void ScriptEngineImpl::CanvasWidthGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* canvas =
      UnwrapHandle<CanvasHandle>(info.GetIsolate(), info.HolderV2(), kCanvasHandleTag);
  if (!canvas) {
    return;
  }
  info.GetReturnValue().Set(canvas->surface->width());
}

void ScriptEngineImpl::CanvasWidthSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* canvas =
      UnwrapHandle<CanvasHandle>(info.GetIsolate(), info.HolderV2(), kCanvasHandleTag);
  if (!canvas) {
    return;
  }

  double width = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &width) ||
      !canvas->surface->Resize(static_cast<int>(width), canvas->surface->height())) {
    ThrowTypeError(info.GetIsolate(), "width must be a positive number");
    return;
  }

  if (auto* engine = From(info.GetIsolate())) {
    engine->RecordDrawOp();
  }
  if (canvas->context_handle) {
    canvas->context_handle->context->ResetState();
  }
}

void ScriptEngineImpl::CanvasHeightGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* canvas =
      UnwrapHandle<CanvasHandle>(info.GetIsolate(), info.HolderV2(), kCanvasHandleTag);
  if (!canvas) {
    return;
  }
  info.GetReturnValue().Set(canvas->surface->height());
}

void ScriptEngineImpl::CanvasHeightSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* canvas =
      UnwrapHandle<CanvasHandle>(info.GetIsolate(), info.HolderV2(), kCanvasHandleTag);
  if (!canvas) {
    return;
  }

  double height = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &height) ||
      !canvas->surface->Resize(canvas->surface->width(), static_cast<int>(height))) {
    ThrowTypeError(info.GetIsolate(), "height must be a positive number");
    return;
  }

  if (auto* engine = From(info.GetIsolate())) {
    engine->RecordDrawOp();
  }
  if (canvas->context_handle) {
    canvas->context_handle->context->ResetState();
  }
}

void ScriptEngineImpl::CanvasSetAttribute(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 2 || !info[0]->IsString()) {
    ThrowTypeError(isolate,
                   "setAttribute(name, value) requires a string name");
    return;
  }

  auto context = isolate->GetCurrentContext();
  info.This()
      ->Set(context, ToV8String(isolate, ToStdString(isolate, info[0])),
            info[1])
      .Check();
}

void ScriptEngineImpl::CanvasAddEventListener(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
}

void ScriptEngineImpl::CanvasRemoveEventListener(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
}


}  // namespace canvas_engine
