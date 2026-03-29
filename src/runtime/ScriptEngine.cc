#include "canvas_engine/runtime/ScriptEngine.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "canvas_engine/canvas/Canvas2DContext.h"
#include "canvas_engine/canvas/CanvasSurface.h"

namespace canvas_engine {

namespace {

struct ContextHandle {
  explicit ContextHandle(std::unique_ptr<Canvas2DContext> ctx)
      : context(std::move(ctx)) {}

  std::unique_ptr<Canvas2DContext> context;
};

constexpr v8::EmbedderDataTypeTag kCanvasHandleTag = 1;
constexpr v8::EmbedderDataTypeTag kContextHandleTag = 2;

template <typename T>
T* Unwrap(v8::Isolate* isolate, v8::Local<v8::Object> object,
          v8::EmbedderDataTypeTag tag) {
  return static_cast<T*>(
      object->GetAlignedPointerFromInternalField(isolate, 0, tag));
}

v8::Local<v8::String> ToV8String(v8::Isolate* isolate, const std::string& value) {
  return v8::String::NewFromUtf8(isolate, value.c_str()).ToLocalChecked();
}

std::string ToStdString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  v8::String::Utf8Value utf8(isolate, value);
  return *utf8 ? *utf8 : "";
}

std::string DetectV8DataDir() {
#if defined(__APPLE__)
  constexpr const char* kCandidateDirs[] = {
      "/opt/homebrew/opt/v8/libexec",
      "/usr/local/opt/v8/libexec",
  };
  for (const char* dir : kCandidateDirs) {
    if (std::filesystem::exists(std::filesystem::path(dir) / "icudtl.dat")) {
      return dir;
    }
  }
#endif
  return {};
}

void V8FatalError(const char* location, int line, const char* message) {
  std::cerr << "V8 fatal error";
  if (location != nullptr) {
    std::cerr << " at " << location;
    if (line > 0) {
      std::cerr << ":" << line;
    }
  }
  if (message != nullptr) {
    std::cerr << ": " << message;
  }
  std::cerr << std::endl;
}

bool ReadFile(const std::string& path, std::string* out_source) {
  std::ifstream input(path, std::ios::in | std::ios::binary);
  if (!input) {
    return false;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  *out_source = buffer.str();
  return true;
}

bool ExtractDouble(const v8::FunctionCallbackInfo<v8::Value>& info, int index,
                   double* out_value) {
  if (info.Length() <= index) {
    return false;
  }
  return info[index]
      ->NumberValue(info.GetIsolate()->GetCurrentContext())
      .To(out_value);
}

bool ExtractDouble(v8::Isolate* isolate, v8::Local<v8::Value> value,
                   double* out_value) {
  return value->NumberValue(isolate->GetCurrentContext()).To(out_value);
}

void ThrowTypeError(v8::Isolate* isolate, const char* message) {
  isolate->ThrowException(
      v8::Exception::TypeError(ToV8String(isolate, message)));
}

void ThrowError(v8::Isolate* isolate, const std::string& message) {
  isolate->ThrowException(
      v8::Exception::Error(ToV8String(isolate, message)));
}

std::string FormatException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
  std::ostringstream output;
  v8::HandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();

  v8::String::Utf8Value exception(isolate, try_catch->Exception());
  if (try_catch->Message().IsEmpty()) {
    output << (*exception ? *exception : "Unknown exception");
    return output.str();
  }

  auto message = try_catch->Message();
  v8::String::Utf8Value script_name(isolate, message->GetScriptOrigin().ResourceName());
  const int line = message->GetLineNumber(context).FromMaybe(0);
  output << (*script_name ? *script_name : "<script>") << ":" << line << ": "
         << (*exception ? *exception : "Unknown exception");

  v8::Local<v8::Value> stack;
  if (try_catch->StackTrace(context).ToLocal(&stack) && stack->IsString()) {
    v8::String::Utf8Value stack_text(isolate, stack);
    output << "\n" << (*stack_text ? *stack_text : "");
  }

  return output.str();
}

void PrintArgs(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  for (int i = 0; i < info.Length(); ++i) {
    if (i > 0) {
      std::fputc(' ', stdout);
    }
    const std::string text = ToStdString(isolate, info[i]);
    std::fwrite(text.data(), 1, text.size(), stdout);
  }
  std::fputc('\n', stdout);
  std::fflush(stdout);
}

}  // namespace

struct ScriptEngine::CanvasHandle {
  explicit CanvasHandle(std::shared_ptr<CanvasSurface> surface_value)
      : surface(std::move(surface_value)) {}

  std::shared_ptr<CanvasSurface> surface;
  std::unique_ptr<ContextHandle> context_handle;
  v8::Global<v8::Object> context_object;
};

ScriptEngine::ScriptEngine() {
  v8::V8::SetFatalErrorHandler(V8FatalError);

  const std::string v8_data_dir = DetectV8DataDir();
  if (!v8_data_dir.empty()) {
    const auto icu_data_path = (std::filesystem::path(v8_data_dir) / "icudtl.dat").string();
    v8::V8::InitializeICU(icu_data_path.c_str());
    if (std::filesystem::exists(std::filesystem::path(v8_data_dir) / "snapshot_blob.bin")) {
      v8::V8::InitializeExternalStartupData(v8_data_dir.c_str());
    }
  } else {
    v8::V8::InitializeICUDefaultLocation(nullptr);
    v8::V8::InitializeExternalStartupData(nullptr);
  }

  platform_ = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform_.get());
  v8::V8::Initialize();

  allocator_.reset(v8::ArrayBuffer::Allocator::NewDefaultAllocator());

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = allocator_.get();
  isolate_ = v8::Isolate::New(create_params);
  isolate_->SetData(0, this);

  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto context = GetContext();
  context_.Reset(isolate_, context);
}

ScriptEngine::~ScriptEngine() {
  {
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);
    context_.Reset();
    canvas_template_.Reset();
    context_2d_template_.Reset();
    canvases_.clear();
  }

  isolate_->Dispose();
  isolate_ = nullptr;
  allocator_.reset();
  v8::V8::Dispose();
  v8::V8::DisposePlatform();
}

bool ScriptEngine::RunScriptFile(const std::string& script_path) {
  std::string source;
  if (!ReadFile(script_path, &source)) {
    std::cerr << "failed to read script: " << script_path << std::endl;
    return false;
  }

  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto context = context_.Get(isolate_);
  v8::Context::Scope context_scope(context);

  v8::TryCatch try_catch(isolate_);
  auto source_string = ToV8String(isolate_, source);
  auto resource_name = ToV8String(isolate_, script_path);
  v8::ScriptOrigin origin(resource_name);

  v8::Local<v8::Script> script;
  if (!v8::Script::Compile(context, source_string, &origin).ToLocal(&script)) {
    std::cerr << FormatException(isolate_, &try_catch) << std::endl;
    return false;
  }

  v8::Local<v8::Value> result;
  if (!script->Run(context).ToLocal(&result)) {
    std::cerr << FormatException(isolate_, &try_catch) << std::endl;
    return false;
  }

  return true;
}

v8::Local<v8::Context> ScriptEngine::GetContext() {
  if (!context_.IsEmpty()) {
    return context_.Get(isolate_);
  }

  auto global = v8::ObjectTemplate::New(isolate_);
  global->Set(v8::String::NewFromUtf8Literal(isolate_, "Canvas"),
              GetCanvasTemplate());
  auto context = v8::Context::New(isolate_, nullptr, global);
  v8::Context::Scope context_scope(context);

  auto print = v8::Function::New(context, PrintCallback).ToLocalChecked();
  context->Global()
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "print"), print)
      .Check();

  auto console = v8::Object::New(isolate_);
  auto log = v8::Function::New(context, ConsoleLogCallback).ToLocalChecked();
  console
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "log"), log)
      .Check();
  context->Global()
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "console"),
            console)
      .Check();

  return context;
}

v8::Local<v8::FunctionTemplate> ScriptEngine::GetCanvasTemplate() {
  if (!canvas_template_.IsEmpty()) {
    return canvas_template_.Get(isolate_);
  }

  auto tpl = v8::FunctionTemplate::New(isolate_, CanvasConstructor);
  tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate_, "Canvas"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "width"), CanvasWidthGetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "height"), CanvasHeightGetter);
  tpl->PrototypeTemplate()->Set(
      isolate_, "getContext",
      v8::FunctionTemplate::New(isolate_, CanvasGetContext));
  tpl->PrototypeTemplate()->Set(
      isolate_, "saveToPng",
      v8::FunctionTemplate::New(isolate_, CanvasSaveToPng));

  canvas_template_.Reset(isolate_, tpl);
  return tpl;
}

v8::Local<v8::FunctionTemplate> ScriptEngine::GetContext2DTemplate() {
  if (!context_2d_template_.IsEmpty()) {
    return context_2d_template_.Get(isolate_);
  }

  auto tpl = v8::FunctionTemplate::New(isolate_);
  tpl->SetClassName(
      v8::String::NewFromUtf8Literal(isolate_, "CanvasRenderingContext2D"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "fillStyle"), FillStyleGetter,
      FillStyleSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "strokeStyle"),
      StrokeStyleGetter, StrokeStyleSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "lineWidth"), LineWidthGetter,
      LineWidthSetter);

  tpl->PrototypeTemplate()->Set(isolate_, "save",
                                v8::FunctionTemplate::New(isolate_, CtxSave));
  tpl->PrototypeTemplate()->Set(isolate_, "restore",
                                v8::FunctionTemplate::New(isolate_, CtxRestore));
  tpl->PrototypeTemplate()->Set(
      isolate_, "translate",
      v8::FunctionTemplate::New(isolate_, CtxTranslate));
  tpl->PrototypeTemplate()->Set(isolate_, "scale",
                                v8::FunctionTemplate::New(isolate_, CtxScale));
  tpl->PrototypeTemplate()->Set(isolate_, "rotate",
                                v8::FunctionTemplate::New(isolate_, CtxRotate));
  tpl->PrototypeTemplate()->Set(
      isolate_, "clearRect",
      v8::FunctionTemplate::New(isolate_, CtxClearRect));
  tpl->PrototypeTemplate()->Set(
      isolate_, "fillRect",
      v8::FunctionTemplate::New(isolate_, CtxFillRect));
  tpl->PrototypeTemplate()->Set(
      isolate_, "strokeRect",
      v8::FunctionTemplate::New(isolate_, CtxStrokeRect));
  tpl->PrototypeTemplate()->Set(
      isolate_, "beginPath",
      v8::FunctionTemplate::New(isolate_, CtxBeginPath));
  tpl->PrototypeTemplate()->Set(
      isolate_, "moveTo",
      v8::FunctionTemplate::New(isolate_, CtxMoveTo));
  tpl->PrototypeTemplate()->Set(
      isolate_, "lineTo",
      v8::FunctionTemplate::New(isolate_, CtxLineTo));
  tpl->PrototypeTemplate()->Set(isolate_, "arc",
                                v8::FunctionTemplate::New(isolate_, CtxArc));
  tpl->PrototypeTemplate()->Set(
      isolate_, "closePath",
      v8::FunctionTemplate::New(isolate_, CtxClosePath));
  tpl->PrototypeTemplate()->Set(isolate_, "fill",
                                v8::FunctionTemplate::New(isolate_, CtxFill));
  tpl->PrototypeTemplate()->Set(isolate_, "stroke",
                                v8::FunctionTemplate::New(isolate_, CtxStroke));

  context_2d_template_.Reset(isolate_, tpl);
  return tpl;
}

ScriptEngine* ScriptEngine::From(v8::Isolate* isolate) {
  return static_cast<ScriptEngine*>(isolate->GetData(0));
}

void ScriptEngine::PrintCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  PrintArgs(info);
}

void ScriptEngine::ConsoleLogCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  PrintArgs(info);
}

void ScriptEngine::CanvasConstructor(
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

  auto surface = CanvasSurface::CreateRaster(static_cast<int>(width),
                                             static_cast<int>(height));
  if (!surface) {
    ThrowError(isolate, "failed to create raster canvas surface");
    return;
  }

  auto* engine = From(isolate);
  engine->canvases_.push_back(std::make_unique<CanvasHandle>(surface));
  auto* handle = engine->canvases_.back().get();
  info.This()->SetAlignedPointerInInternalField(0, handle, kCanvasHandleTag);
  info.GetReturnValue().Set(info.This());
}

void ScriptEngine::CanvasGetContext(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  auto* canvas = Unwrap<CanvasHandle>(isolate, info.This(), kCanvasHandleTag);
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
        std::make_unique<Canvas2DContext>(canvas->surface));
  }

  if (canvas->context_object.IsEmpty()) {
    auto object = engine->GetContext2DTemplate()
                      ->InstanceTemplate()
                      ->NewInstance(context)
                      .ToLocalChecked();
    object->SetAlignedPointerInInternalField(0, canvas->context_handle.get(),
                                             kContextHandleTag);
    canvas->context_object.Reset(isolate, object);
  }

  info.GetReturnValue().Set(canvas->context_object.Get(isolate));
}

void ScriptEngine::CanvasSaveToPng(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  auto* canvas = Unwrap<CanvasHandle>(isolate, info.This(), kCanvasHandleTag);
  if (!canvas) {
    ThrowTypeError(isolate, "invalid Canvas instance");
    return;
  }

  if (info.Length() < 1 || !info[0]->IsString()) {
    ThrowTypeError(isolate, "saveToPng(path) requires a string path");
    return;
  }

  const std::string output_path = ToStdString(isolate, info[0]);
  if (!canvas->surface->SavePng(output_path)) {
    ThrowError(isolate, "failed to write png: " + output_path);
  }
}

void ScriptEngine::CanvasWidthGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* canvas =
      Unwrap<CanvasHandle>(info.GetIsolate(), info.HolderV2(), kCanvasHandleTag);
  if (!canvas) {
    return;
  }
  info.GetReturnValue().Set(canvas->surface->width());
}

void ScriptEngine::CanvasHeightGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* canvas =
      Unwrap<CanvasHandle>(info.GetIsolate(), info.HolderV2(), kCanvasHandleTag);
  if (!canvas) {
    return;
  }
  info.GetReturnValue().Set(canvas->surface->height());
}

void ScriptEngine::FillStyleGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(
      ToV8String(info.GetIsolate(), handle->context->fill_style()));
}

void ScriptEngine::FillStyleSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  if (!value->IsString() ||
      !handle->context->SetFillStyle(ToStdString(info.GetIsolate(), value))) {
    ThrowTypeError(info.GetIsolate(), "fillStyle must be a valid CSS color");
  }
}

void ScriptEngine::StrokeStyleGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(
      ToV8String(info.GetIsolate(), handle->context->stroke_style()));
}

void ScriptEngine::StrokeStyleSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  if (!value->IsString() ||
      !handle->context->SetStrokeStyle(ToStdString(info.GetIsolate(), value))) {
    ThrowTypeError(info.GetIsolate(), "strokeStyle must be a valid CSS color");
  }
}

void ScriptEngine::LineWidthGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(handle->context->line_width());
}

void ScriptEngine::LineWidthSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }

  double line_width = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &line_width) ||
      !handle->context->SetLineWidth(static_cast<float>(line_width))) {
    ThrowTypeError(info.GetIsolate(), "lineWidth must be a positive number");
  }
}

void ScriptEngine::CtxSave(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->Save();
  }
}

void ScriptEngine::CtxRestore(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->Restore();
  }
}

void ScriptEngine::CtxTranslate(
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

void ScriptEngine::CtxScale(const v8::FunctionCallbackInfo<v8::Value>& info) {
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

void ScriptEngine::CtxRotate(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  double radians = 0.0;
  if (!handle || !ExtractDouble(info, 0, &radians)) {
    ThrowTypeError(info.GetIsolate(), "rotate(radians) requires a number");
    return;
  }
  handle->context->Rotate(static_cast<float>(radians));
}

void ScriptEngine::CtxClearRect(
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

void ScriptEngine::CtxFillRect(
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

void ScriptEngine::CtxStrokeRect(
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

void ScriptEngine::CtxBeginPath(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->BeginPath();
  }
}

void ScriptEngine::CtxMoveTo(const v8::FunctionCallbackInfo<v8::Value>& info) {
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

void ScriptEngine::CtxLineTo(const v8::FunctionCallbackInfo<v8::Value>& info) {
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

void ScriptEngine::CtxArc(const v8::FunctionCallbackInfo<v8::Value>& info) {
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

void ScriptEngine::CtxClosePath(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->ClosePath();
  }
}

void ScriptEngine::CtxFill(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->Fill();
  }
}

void ScriptEngine::CtxStroke(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->Stroke();
  }
}

}  // namespace canvas_engine
