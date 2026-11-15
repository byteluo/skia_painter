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

v8::Local<v8::Context> ScriptEngineImpl::GetContext() {
  if (!context_.IsEmpty()) {
    return context_.Get(isolate_);
  }

  auto global_template = v8::ObjectTemplate::New(isolate_);
  global_template->Set(v8::String::NewFromUtf8Literal(isolate_, "Canvas"),
                       GetCanvasTemplate());
  global_template->Set(v8::String::NewFromUtf8Literal(isolate_, "Image"),
                       GetImageTemplate());
  global_template->Set(v8::String::NewFromUtf8Literal(isolate_, "Path2D"),
                       GetPath2DTemplate());

  auto context = v8::Context::New(isolate_, nullptr, global_template);
  v8::Context::Scope context_scope(context);
  auto global = context->Global();

  auto print = v8::Function::New(context, PrintCallback).ToLocalChecked();
  global->Set(context, v8::String::NewFromUtf8Literal(isolate_, "print"), print)
      .Check();

  auto console = v8::Object::New(isolate_);
  auto log = v8::Function::New(context, ConsoleLogCallback).ToLocalChecked();
  console->Set(context, v8::String::NewFromUtf8Literal(isolate_, "log"), log)
      .Check();
  console->Set(context, v8::String::NewFromUtf8Literal(isolate_, "warn"), log)
      .Check();
  console->Set(context, v8::String::NewFromUtf8Literal(isolate_, "error"), log)
      .Check();
  global->Set(context, v8::String::NewFromUtf8Literal(isolate_, "console"),
              console)
      .Check();
  if (options_.expose_gc) {
    global
        ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "gc"),
              v8::Function::New(context, GcCallback).ToLocalChecked())
        .Check();
  }

  global
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "loadScript"),
            v8::Function::New(context, LoadScriptCallback).ToLocalChecked())
      .Check();
  global
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "setTimeout"),
            v8::Function::New(context, SetTimeoutCallback).ToLocalChecked())
      .Check();
  global
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "clearTimeout"),
            v8::Function::New(context, ClearTimeoutCallback).ToLocalChecked())
      .Check();
  global
      ->Set(context,
            v8::String::NewFromUtf8Literal(isolate_, "requestAnimationFrame"),
            v8::Function::New(context, RequestAnimationFrameCallback)
                .ToLocalChecked())
      .Check();
  global
      ->Set(context,
            v8::String::NewFromUtf8Literal(isolate_, "cancelAnimationFrame"),
            v8::Function::New(context, CancelAnimationFrameCallback)
                .ToLocalChecked())
      .Check();

  auto performance = v8::Object::New(isolate_);
  performance
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "now"),
            v8::Function::New(context, PerformanceNowCallback)
                .ToLocalChecked())
      .Check();
  global
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "performance"),
            performance)
      .Check();
  global
      ->Set(context,
            v8::String::NewFromUtf8Literal(isolate_,
                                           "__canvasEngineClockEpochMs"),
            v8::Number::New(isolate_, clock_epoch_ms_))
      .Check();
  auto date_bootstrap = v8::Script::Compile(
                            context, ToV8String(isolate_, kVirtualDateBootstrap))
                            .ToLocalChecked();
  date_bootstrap->Run(context).ToLocalChecked();
  global
      ->Set(context,
            v8::String::NewFromUtf8Literal(isolate_,
                                           "__canvasEngineHandleCounts"),
            v8::Function::New(context, HandleCountsCallback).ToLocalChecked())
      .Check();

  auto navigator = v8::Object::New(isolate_);
  navigator
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "userAgent"),
            v8::String::NewFromUtf8Literal(isolate_, "canvas_engine"))
      .Check();
  navigator
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "platform"),
#if defined(__APPLE__)
            v8::String::NewFromUtf8Literal(isolate_, "darwin"))
#elif defined(__linux__)
            v8::String::NewFromUtf8Literal(isolate_, "linux"))
#else
            v8::String::NewFromUtf8Literal(isolate_, "unknown"))
#endif
      .Check();
  navigator
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "language"),
            v8::String::NewFromUtf8Literal(isolate_, "en-US"))
      .Check();
  global->Set(context, v8::String::NewFromUtf8Literal(isolate_, "navigator"),
              navigator)
      .Check();

  global->Set(context, v8::String::NewFromUtf8Literal(isolate_, "window"),
              global)
      .Check();
  global->Set(context, v8::String::NewFromUtf8Literal(isolate_, "self"), global)
      .Check();
  global
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "global"), global)
      .Check();
  global
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "devicePixelRatio"),
            v8::Number::New(isolate_, GetExposedDevicePixelRatio()))
      .Check();

  return context;
}

ScriptEngineImpl* ScriptEngineImpl::From(v8::Isolate* isolate) {
  return static_cast<ScriptEngineImpl*>(isolate->GetData(0));
}

void* ScriptEngineImpl::LookupHandle(
    v8::Isolate* isolate, v8::Local<v8::Object> object,
    v8::EmbedderDataTypeTag tag) const {
  if (object.IsEmpty() || object->InternalFieldCount() == 0 ||
      !handle_registry_) {
    return nullptr;
  }
  auto* header = static_cast<HandleRegistry::Header*>(
      object->GetAlignedPointerFromInternalField(isolate, 0, tag));
  return handle_registry_->Lookup(header, static_cast<std::uint32_t>(tag));
}

void ScriptEngineImpl::HandleWeakFinalizer(
    const v8::WeakCallbackInfo<HandleFinalizerData>& data) {
  auto* finalizer_data = data.GetParameter();
  if (finalizer_data == nullptr || finalizer_data->engine == nullptr) {
    return;
  }

  auto* engine = finalizer_data->engine;
  switch (finalizer_data->kind) {
    case HandleFinalizerData::Kind::kCanvas:
      engine->handle_registry_->Remove(finalizer_data->id, kCanvasHandleTag);
      if (auto it = engine->canvases_.find(finalizer_data->id);
          it != engine->canvases_.end()) {
        it->second->object.ClearWeak();
        it->second->object.Reset();
        it->second->context_object.Reset();
        engine->canvases_.erase(it);
      }
      break;
    case HandleFinalizerData::Kind::kGradient:
      engine->handle_registry_->Remove(finalizer_data->id, kGradientHandleTag);
      if (auto it = engine->gradients_.find(finalizer_data->id);
          it != engine->gradients_.end()) {
        it->second->object.ClearWeak();
        it->second->object.Reset();
        engine->gradients_.erase(it);
      }
      break;
    case HandleFinalizerData::Kind::kImage:
      engine->handle_registry_->Remove(finalizer_data->id, kImageHandleTag);
      if (auto it = engine->images_.find(finalizer_data->id);
          it != engine->images_.end()) {
        it->second->object.ClearWeak();
        it->second->object.Reset();
        engine->images_.erase(it);
      }
      break;
    case HandleFinalizerData::Kind::kPath2D:
      engine->handle_registry_->Remove(finalizer_data->id, kPath2DHandleTag);
      if (auto it = engine->path2ds_.find(finalizer_data->id);
          it != engine->path2ds_.end()) {
        it->second->object.ClearWeak();
        it->second->object.Reset();
        engine->path2ds_.erase(it);
      }
      break;
    case HandleFinalizerData::Kind::kPattern:
      engine->handle_registry_->Remove(finalizer_data->id, kPatternHandleTag);
      if (auto it = engine->patterns_.find(finalizer_data->id);
          it != engine->patterns_.end()) {
        it->second->object.ClearWeak();
        it->second->object.Reset();
        engine->patterns_.erase(it);
      }
      break;
  }
}

void ScriptEngineImpl::ContextObjectWeakFinalizer(
    const v8::WeakCallbackInfo<ContextObjectFinalizerData>& data) {
  auto* finalizer_data = data.GetParameter();
  if (finalizer_data == nullptr || finalizer_data->engine == nullptr) {
    return;
  }

  auto canvas = finalizer_data->engine->canvases_.find(finalizer_data->canvas_id);
  if (canvas == finalizer_data->engine->canvases_.end()) {
    return;
  }
  canvas->second->context_object.ClearWeak();
  canvas->second->context_object.Reset();
  canvas->second->context_finalizer_data.reset();
}

void ScriptEngineImpl::PrintCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  PrintArgs(info);
}

void ScriptEngineImpl::ConsoleLogCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  PrintArgs(info);
}

void ScriptEngineImpl::GcCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetIsolate()->LowMemoryNotification();
}

void ScriptEngineImpl::LoadScriptCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1 || !info[0]->IsString()) {
    ThrowTypeError(isolate, "loadScript(path) requires a string path");
    return;
  }

  auto* engine = From(isolate);
  const auto resolved = engine->ResolveScriptPath(ToStdString(isolate, info[0]));
  v8::Local<v8::Value> result;
  if (!engine->ExecuteScriptFile(resolved, &result)) {
    ThrowError(isolate, engine->last_error_);
    return;
  }

  info.GetReturnValue().Set(result);
}

void ScriptEngineImpl::SetTimeoutCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    ThrowTypeError(isolate, "setTimeout(callback, delay?) requires a function");
    return;
  }

  auto* engine = From(isolate);
  double delay_ms = 0.0;
  if (info.Length() > 1 && !info[1]->IsUndefined() && !info[1]->IsNull()) {
    ExtractDouble(info, 1, &delay_ms);
  }
  delay_ms = std::max(0.0, delay_ms);
  std::vector<v8::Local<v8::Value>> arguments;
  for (int i = 2; i < info.Length(); ++i) {
    arguments.push_back(info[i]);
  }
  const std::uint32_t timer_id = engine->EnqueueTask(
      info[0].As<v8::Function>(), false, engine->NowMs() + delay_ms,
      arguments);
  info.GetReturnValue().Set(v8::Integer::NewFromUnsigned(isolate, timer_id));
}

void ScriptEngineImpl::ClearTimeoutCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* engine = From(info.GetIsolate());
  double timer_id = 0.0;
  if (engine && info.Length() > 0 && ExtractDouble(info, 0, &timer_id)) {
    engine->CancelTask(static_cast<std::uint32_t>(std::lround(timer_id)));
  }
  info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
}

void ScriptEngineImpl::RequestAnimationFrameCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    ThrowTypeError(isolate,
                   "requestAnimationFrame(callback) requires a function");
    return;
  }

  auto* engine = From(isolate);
  const std::uint32_t timer_id = engine->EnqueueTask(
      info[0].As<v8::Function>(), true,
      std::max(engine->NowMs(), engine->event_loop_->NextFrameMs()));
  info.GetReturnValue().Set(v8::Integer::NewFromUnsigned(isolate, timer_id));
}

void ScriptEngineImpl::CancelAnimationFrameCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* engine = From(info.GetIsolate());
  double timer_id = 0.0;
  if (engine && info.Length() > 0 && ExtractDouble(info, 0, &timer_id)) {
    engine->CancelTask(static_cast<std::uint32_t>(std::lround(timer_id)));
  }
  info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
}

void ScriptEngineImpl::PerformanceNowCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* engine = From(info.GetIsolate());
  info.GetReturnValue().Set(
      v8::Number::New(info.GetIsolate(), engine->NowMs()));
}

void ScriptEngineImpl::HandleCountsCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  auto* engine = From(isolate);
  auto context = isolate->GetCurrentContext();
  auto counts = v8::Object::New(isolate);
  counts
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "canvases"),
            v8::Integer::NewFromUnsigned(
                isolate, static_cast<std::uint32_t>(engine->canvases_.size())))
      .Check();
  counts
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "gradients"),
            v8::Integer::NewFromUnsigned(
                isolate, static_cast<std::uint32_t>(engine->gradients_.size())))
      .Check();
  counts
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "images"),
            v8::Integer::NewFromUnsigned(
                isolate, static_cast<std::uint32_t>(engine->images_.size())))
      .Check();
  counts
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "patterns"),
            v8::Integer::NewFromUnsigned(
                isolate, static_cast<std::uint32_t>(engine->patterns_.size())))
      .Check();
  counts
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "paths"),
            v8::Integer::NewFromUnsigned(
                isolate, static_cast<std::uint32_t>(engine->path2ds_.size())))
      .Check();
  info.GetReturnValue().Set(counts);
}


}  // namespace canvas_engine
