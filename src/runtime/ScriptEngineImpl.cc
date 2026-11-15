#include "ScriptEngineImpl.h"

#include "bindings/BindingSupport.h"
#include "ScriptLoader.h"

#include <chrono>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "canvas_engine/canvas/Canvas2DContext.h"
#include "canvas_engine/canvas/CanvasSurface.h"
#include "canvas_engine/canvas/ColorParser.h"
#include "canvas_engine/canvas/ImageAsset.h"
#include "canvas_engine/runtime/V8Platform.h"
#include "EventLoop.h"
#include "HandleRegistry.h"
#include "include/effects/SkGradient.h"

namespace canvas_engine {

using namespace binding_internal;

ScriptEngineImpl::ScriptEngineImpl()
    : ScriptEngineImpl(Options{GetDefaultDevicePixelRatio(), false}) {}

ScriptEngineImpl::ScriptEngineImpl(Options options) : options_(options) {
  if (!(options_.device_pixel_ratio > 0.0f) ||
      !std::isfinite(options_.device_pixel_ratio)) {
    throw std::invalid_argument("device_pixel_ratio must be positive");
  }
  if (options_.clock_epoch_ms.has_value()) {
    if (!std::isfinite(*options_.clock_epoch_ms)) {
      throw std::invalid_argument("clock_epoch_ms must be finite");
    }
    clock_epoch_ms_ = *options_.clock_epoch_ms;
  } else {
    clock_epoch_ms_ = std::chrono::duration<double, std::milli>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  }
  std::filesystem::path font_dir = options_.font_dir;
  if (font_dir.empty()) {
    if (const char* environment_font_dir =
            std::getenv("CANVAS_ENGINE_FONT_DIR");
        environment_font_dir != nullptr && *environment_font_dir != '\0') {
      font_dir = environment_font_dir;
    }
  }
  if (font_dir.empty() && CANVAS_ENGINE_DEFAULT_FONT_DIR[0] != '\0') {
    font_dir = CANVAS_ENGINE_DEFAULT_FONT_DIR;
  }
  bool allow_system_fonts = options_.allow_system_fonts;
#if !CANVAS_ENGINE_BUNDLED_FONTS
  allow_system_fonts = true;
#endif
  font_manager_ = std::make_shared<FontManager>(font_dir, allow_system_fonts);
  handle_registry_ = std::make_unique<HandleRegistry>();
  V8Platform::Initialize();

  allocator_.reset(v8::ArrayBuffer::Allocator::NewDefaultAllocator());

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = allocator_.get();
  isolate_ = v8::Isolate::New(create_params);
  isolate_->SetData(0, this);
  event_loop_ = std::make_unique<EventLoop>(
      isolate_, options_.realtime_clock, engine_start_time_);

  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto context = GetContext();
  context_.Reset(isolate_, context);
}

ScriptEngineImpl::~ScriptEngineImpl() {
  {
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);
    context_.Reset();
    last_result_.Reset();
    canvas_template_.Reset();
    context_2d_template_.Reset();
    gradient_template_.Reset();
    image_template_.Reset();
    path_2d_template_.Reset();
    pattern_template_.Reset();
    canvases_.clear();
    gradients_.clear();
    images_.clear();
    path2ds_.clear();
    patterns_.clear();
    event_loop_.reset();
    handle_registry_.reset();
  }

  isolate_->Dispose();
  isolate_ = nullptr;
  allocator_.reset();
}

bool ScriptEngineImpl::RunScriptFile(const std::string& script_path) {
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto context = context_.Get(isolate_);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Value> result;
  const auto resolved = ResolveScriptPath(script_path);
  if (!ExecuteScriptFile(resolved, &result)) {
    return false;
  }

  return true;
}

bool ScriptEngineImpl::RunScriptSource(const std::string& source,
                                   const std::string& resource_name) {
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto context = context_.Get(isolate_);
  v8::Context::Scope context_scope(context);

  const std::string effective_name =
      resource_name.empty() ? "<source>" : resource_name;
  return ExecuteScriptSource(source, ResolveScriptPath(effective_name),
                             nullptr);
}

std::string ScriptEngineImpl::last_result_json() const {
  if (last_result_.IsEmpty()) {
    return "null";
  }

  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto context = context_.Get(isolate_);
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> json;
  if (!v8::JSON::Stringify(context, last_result_.Get(isolate_))
           .ToLocal(&json)) {
    return "null";
  }
  return ToStdString(isolate_, json);
}

bool ScriptEngineImpl::ExecuteScriptFile(const std::filesystem::path& script_path,
                                     v8::Local<v8::Value>* out_result) {
  std::string source;
  if (!ScriptLoader::LoadScript(script_path, &source)) {
    last_error_ = "failed to read script: " + script_path.string();
    return false;
  }

  return ExecuteScriptSource(source, script_path, out_result);
}

bool ScriptEngineImpl::ExecuteScriptSource(
    const std::string& source,
    const std::filesystem::path& resource_path,
    v8::Local<v8::Value>* out_result) {
  last_error_.clear();
  auto context = context_.Get(isolate_);
  v8::Context::Scope context_scope(context);
  v8::TryCatch try_catch(isolate_);

  script_stack_.push_back(resource_path);
  const auto pop_path = [this]() { script_stack_.pop_back(); };

  auto source_string = ToV8String(isolate_, source);
  auto resource_name = ToV8String(isolate_, resource_path.string());
  v8::ScriptOrigin origin(resource_name);

  v8::Local<v8::Script> script;
  if (!v8::Script::Compile(context, source_string, &origin).ToLocal(&script)) {
    last_error_ = FormatException(isolate_, &try_catch);
    pop_path();
    return false;
  }

  v8::Local<v8::Value> result;
  if (!script->Run(context).ToLocal(&result)) {
    last_error_ = FormatException(isolate_, &try_catch);
    pop_path();
    return false;
  }

  pop_path();
  if (!DrainPendingTasks()) {
    return false;
  }
  last_result_.Reset(isolate_, result);
  context->Global()
      ->Set(context,
            v8::String::NewFromUtf8Literal(isolate_,
                                           "__canvasEngineLastScriptResult"),
            result)
      .Check();
  if (out_result != nullptr) {
    *out_result = result;
  }
  return true;
}

bool ScriptEngineImpl::DrainPendingTasks() {
  return event_loop_->DrainPendingTasks(
      context_.Get(isolate_), &last_error_,
      [this](v8::TryCatch* try_catch) {
        return FormatException(isolate_, try_catch);
      });
}

std::uint32_t ScriptEngineImpl::EnqueueTask(
    v8::Local<v8::Function> callback, bool animation_frame,
    double ready_at_ms,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  return event_loop_->EnqueueTask(callback, animation_frame, ready_at_ms,
                                  arguments);
}

std::uint32_t ScriptEngineImpl::EnqueueNativeTask(std::function<void()> callback,
                                              double ready_at_ms) {
  return event_loop_->EnqueueNativeTask(std::move(callback), ready_at_ms);
}

double ScriptEngineImpl::NowMs() const { return event_loop_->NowMs(); }

void ScriptEngineImpl::CancelTask(std::uint32_t task_id) {
  event_loop_->CancelTask(task_id);
}

void ScriptEngineImpl::RecordDrawOp() { event_loop_->RecordDrawOp(); }

std::uint64_t ScriptEngineImpl::NextHandleId() {
  return next_handle_id_++;
}

std::filesystem::path ScriptEngineImpl::ResolveScriptPath(
    const std::string& script_path) const {
  const auto base = script_stack_.empty() ? std::filesystem::current_path()
                                          : script_stack_.back().parent_path();
  return ScriptLoader::ResolvePath(script_path, base);
}

}  // namespace canvas_engine
