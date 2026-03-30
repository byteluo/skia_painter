#include "canvas_engine/runtime/ScriptEngine.h"

#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "canvas_engine/canvas/Canvas2DContext.h"
#include "canvas_engine/canvas/CanvasSurface.h"
#include "canvas_engine/canvas/ColorParser.h"
#include "include/effects/SkGradient.h"

namespace canvas_engine {

namespace {

struct ContextHandle {
  explicit ContextHandle(std::unique_ptr<Canvas2DContext> ctx)
      : context(std::move(ctx)) {}

  std::unique_ptr<Canvas2DContext> context;
};

constexpr v8::EmbedderDataTypeTag kCanvasHandleTag = 1;
constexpr v8::EmbedderDataTypeTag kContextHandleTag = 2;
constexpr v8::EmbedderDataTypeTag kGradientHandleTag = 3;

using Clock = std::chrono::steady_clock;

template <typename T>
T* Unwrap(v8::Isolate* isolate, v8::Local<v8::Object> object,
          v8::EmbedderDataTypeTag tag) {
  return static_cast<T*>(
      object->GetAlignedPointerFromInternalField(isolate, 0, tag));
}

v8::Local<v8::String> ToV8String(v8::Isolate* isolate,
                                 const std::string& value) {
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

bool ExtractFloat(v8::Isolate* isolate, v8::Local<v8::Value> value,
                  float* out_value) {
  double raw = 0.0;
  if (!ExtractDouble(isolate, value, &raw)) {
    return false;
  }
  *out_value = static_cast<float>(raw);
  return true;
}

void ThrowTypeError(v8::Isolate* isolate, const char* message) {
  isolate->ThrowException(
      v8::Exception::TypeError(ToV8String(isolate, message)));
}

void ThrowError(v8::Isolate* isolate, const std::string& message) {
  isolate->ThrowException(v8::Exception::Error(ToV8String(isolate, message)));
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
  v8::String::Utf8Value script_name(
      isolate, message->GetScriptOrigin().ResourceName());
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

double NowMilliseconds() {
  const auto now = Clock::now().time_since_epoch();
  return std::chrono::duration<double, std::milli>(now).count();
}

float GetDefaultDevicePixelRatio() {
  if (const char* env = std::getenv("CANVAS_ENGINE_DPR")) {
    try {
      const float value = std::stof(env);
      if (value > 0.0f) {
        return value;
      }
    } catch (...) {
    }
  }
  return 3.0f;
}

float GetExposedDevicePixelRatio() {
  return 1.0f;
}

bool ExtractSixNumbers(const v8::FunctionCallbackInfo<v8::Value>& info,
                       float* a, float* b, float* c, float* d, float* e,
                       float* f) {
  double values[6] = {0, 0, 0, 0, 0, 0};
  for (int i = 0; i < 6; ++i) {
    if (!ExtractDouble(info, i, &values[i])) {
      return false;
    }
  }
  *a = static_cast<float>(values[0]);
  *b = static_cast<float>(values[1]);
  *c = static_cast<float>(values[2]);
  *d = static_cast<float>(values[3]);
  *e = static_cast<float>(values[4]);
  *f = static_cast<float>(values[5]);
  return true;
}

}  // namespace

struct ScriptEngine::CanvasHandle {
  explicit CanvasHandle(std::shared_ptr<CanvasSurface> surface_value)
      : surface(std::move(surface_value)) {}

  std::shared_ptr<CanvasSurface> surface;
  std::unique_ptr<ContextHandle> context_handle;
  v8::Global<v8::Object> context_object;
};

struct ScriptEngine::GradientHandle {
  enum class Kind {
    kLinear,
    kRadial,
  };

  struct ColorStop {
    float offset = 0.0f;
    SkColor color = SK_ColorBLACK;
  };

  explicit GradientHandle(Kind gradient_kind) : kind(gradient_kind) {}

  sk_sp<SkShader> BuildShader() const {
    if (stops.empty()) {
      return nullptr;
    }

    std::vector<ColorStop> sorted = stops;
    std::sort(sorted.begin(), sorted.end(),
              [](const ColorStop& lhs, const ColorStop& rhs) {
                return lhs.offset < rhs.offset;
              });

    std::vector<SkScalar> positions;
    std::vector<SkColor4f> colors4f;
    positions.reserve(sorted.size());
    colors4f.reserve(sorted.size());
    for (const auto& stop : sorted) {
      colors4f.push_back(SkColor4f::FromColor(stop.color));
      positions.push_back(stop.offset);
    }

    SkGradient gradient(
        SkGradient::Colors(colors4f, positions, SkTileMode::kClamp),
        SkGradient::Interpolation());

    if (kind == Kind::kLinear) {
      const SkPoint points[2] = {SkPoint::Make(x0, y0), SkPoint::Make(x1, y1)};
      return SkShaders::LinearGradient(points, gradient);
    }

    return SkShaders::TwoPointConicalGradient(SkPoint::Make(x0, y0), r0,
                                              SkPoint::Make(x1, y1), r1,
                                              gradient);
  }

  Kind kind;
  float x0 = 0.0f;
  float y0 = 0.0f;
  float r0 = 0.0f;
  float x1 = 0.0f;
  float y1 = 0.0f;
  float r1 = 0.0f;
  std::vector<ColorStop> stops;
};

ScriptEngine::ScriptEngine() {
  v8::V8::SetFatalErrorHandler(V8FatalError);

  const std::string v8_data_dir = DetectV8DataDir();
  if (!v8_data_dir.empty()) {
    const auto icu_data_path =
        (std::filesystem::path(v8_data_dir) / "icudtl.dat").string();
    v8::V8::InitializeICU(icu_data_path.c_str());
    if (std::filesystem::exists(std::filesystem::path(v8_data_dir) /
                                "snapshot_blob.bin")) {
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
    gradient_template_.Reset();
    canvases_.clear();
    gradients_.clear();
  }

  isolate_->Dispose();
  isolate_ = nullptr;
  allocator_.reset();
  v8::V8::Dispose();
  v8::V8::DisposePlatform();
}

bool ScriptEngine::RunScriptFile(const std::string& script_path) {
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto context = context_.Get(isolate_);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Value> result;
  const auto resolved = ResolveScriptPath(script_path);
  if (!ExecuteScriptFile(resolved, &result)) {
    std::cerr << last_error_ << std::endl;
    return false;
  }

  return true;
}

bool ScriptEngine::ExecuteScriptFile(const std::filesystem::path& script_path,
                                     v8::Local<v8::Value>* out_result) {
  last_error_.clear();

  std::string source;
  if (!ReadFile(script_path.string(), &source)) {
    last_error_ = "failed to read script: " + script_path.string();
    return false;
  }

  auto context = context_.Get(isolate_);
  v8::Context::Scope context_scope(context);
  v8::TryCatch try_catch(isolate_);

  script_stack_.push_back(script_path);
  const auto pop_path = [this]() { script_stack_.pop_back(); };

  auto source_string = ToV8String(isolate_, source);
  auto resource_name = ToV8String(isolate_, script_path.string());
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
  if (out_result != nullptr) {
    *out_result = result;
  }
  return true;
}

std::filesystem::path ScriptEngine::ResolveScriptPath(
    const std::string& script_path) const {
  std::filesystem::path path(script_path);
  if (path.is_absolute()) {
    return path.lexically_normal();
  }

  const auto base = script_stack_.empty() ? std::filesystem::current_path()
                                          : script_stack_.back().parent_path();
  return (base / path).lexically_normal();
}

v8::Local<v8::Context> ScriptEngine::GetContext() {
  if (!context_.IsEmpty()) {
    return context_.Get(isolate_);
  }

  auto global_template = v8::ObjectTemplate::New(isolate_);
  global_template->Set(v8::String::NewFromUtf8Literal(isolate_, "Canvas"),
                       GetCanvasTemplate());

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

  auto navigator = v8::Object::New(isolate_);
  navigator
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "userAgent"),
            v8::String::NewFromUtf8Literal(isolate_, "canvas_engine"))
      .Check();
  navigator
      ->Set(context, v8::String::NewFromUtf8Literal(isolate_, "platform"),
            v8::String::NewFromUtf8Literal(isolate_, "darwin"))
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

v8::Local<v8::FunctionTemplate> ScriptEngine::GetCanvasTemplate() {
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
  tpl->PrototypeTemplate()->Set(
      isolate_, "getContext",
      v8::FunctionTemplate::New(isolate_, CanvasGetContext));
  tpl->PrototypeTemplate()->Set(
      isolate_, "saveToPng",
      v8::FunctionTemplate::New(isolate_, CanvasSaveToPng));
  tpl->PrototypeTemplate()->Set(
      isolate_, "setAttribute",
      v8::FunctionTemplate::New(isolate_, CanvasSetAttribute));
  tpl->PrototypeTemplate()->Set(
      isolate_, "addEventListener",
      v8::FunctionTemplate::New(isolate_, CanvasAddEventListener));
  tpl->PrototypeTemplate()->Set(
      isolate_, "removeEventListener",
      v8::FunctionTemplate::New(isolate_, CanvasRemoveEventListener));

  canvas_template_.Reset(isolate_, tpl);
  return tpl;
}

v8::Local<v8::FunctionTemplate> ScriptEngine::GetContext2DTemplate() {
  if (!context_2d_template_.IsEmpty()) {
    return context_2d_template_.Get(isolate_);
  }

  auto tpl = v8::FunctionTemplate::New(isolate_);
  tpl->SetClassName(
      v8::String::NewFromUtf8Literal(isolate_,
                                     "CanvasRenderingContext2D"));
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
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "font"), FontGetter,
      FontSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "textAlign"), TextAlignGetter,
      TextAlignSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "textBaseline"),
      TextBaselineGetter, TextBaselineSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "globalAlpha"),
      GlobalAlphaGetter, GlobalAlphaSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "globalCompositeOperation"),
      GlobalCompositeOperationGetter, GlobalCompositeOperationSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "lineCap"), LineCapGetter,
      LineCapSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "lineJoin"), LineJoinGetter,
      LineJoinSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "miterLimit"),
      MiterLimitGetter, MiterLimitSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "shadowBlur"),
      ShadowBlurGetter, ShadowBlurSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "shadowColor"),
      ShadowColorGetter, ShadowColorSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "shadowOffsetX"),
      ShadowOffsetXGetter, ShadowOffsetXSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "shadowOffsetY"),
      ShadowOffsetYGetter, ShadowOffsetYSetter);
  tpl->InstanceTemplate()->SetNativeDataProperty(
      v8::String::NewFromUtf8Literal(isolate_, "lineDashOffset"),
      LineDashOffsetGetter, LineDashOffsetSetter);

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
      isolate_, "transform",
      v8::FunctionTemplate::New(isolate_, CtxTransform));
  tpl->PrototypeTemplate()->Set(
      isolate_, "setTransform",
      v8::FunctionTemplate::New(isolate_, CtxSetTransform));
  tpl->PrototypeTemplate()->Set(
      isolate_, "resetTransform",
      v8::FunctionTemplate::New(isolate_, CtxResetTransform));
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
  tpl->PrototypeTemplate()->Set(
      isolate_, "bezierCurveTo",
      v8::FunctionTemplate::New(isolate_, CtxBezierCurveTo));
  tpl->PrototypeTemplate()->Set(isolate_, "rect",
                                v8::FunctionTemplate::New(isolate_, CtxRect));
  tpl->PrototypeTemplate()->Set(isolate_, "arc",
                                v8::FunctionTemplate::New(isolate_, CtxArc));
  tpl->PrototypeTemplate()->Set(
      isolate_, "closePath",
      v8::FunctionTemplate::New(isolate_, CtxClosePath));
  tpl->PrototypeTemplate()->Set(isolate_, "clip",
                                v8::FunctionTemplate::New(isolate_, CtxClip));
  tpl->PrototypeTemplate()->Set(isolate_, "fill",
                                v8::FunctionTemplate::New(isolate_, CtxFill));
  tpl->PrototypeTemplate()->Set(isolate_, "stroke",
                                v8::FunctionTemplate::New(isolate_, CtxStroke));
  tpl->PrototypeTemplate()->Set(
      isolate_, "setLineDash",
      v8::FunctionTemplate::New(isolate_, CtxSetLineDash));
  tpl->PrototypeTemplate()->Set(
      isolate_, "getLineDash",
      v8::FunctionTemplate::New(isolate_, CtxGetLineDash));
  tpl->PrototypeTemplate()->Set(
      isolate_, "createLinearGradient",
      v8::FunctionTemplate::New(isolate_, CtxCreateLinearGradient));
  tpl->PrototypeTemplate()->Set(
      isolate_, "createRadialGradient",
      v8::FunctionTemplate::New(isolate_, CtxCreateRadialGradient));
  tpl->PrototypeTemplate()->Set(
      isolate_, "measureText",
      v8::FunctionTemplate::New(isolate_, CtxMeasureText));
  tpl->PrototypeTemplate()->Set(
      isolate_, "fillText",
      v8::FunctionTemplate::New(isolate_, CtxFillText));
  tpl->PrototypeTemplate()->Set(
      isolate_, "strokeText",
      v8::FunctionTemplate::New(isolate_, CtxStrokeText));

  context_2d_template_.Reset(isolate_, tpl);
  return tpl;
}

v8::Local<v8::FunctionTemplate> ScriptEngine::GetGradientTemplate() {
  if (!gradient_template_.IsEmpty()) {
    return gradient_template_.Get(isolate_);
  }

  auto tpl = v8::FunctionTemplate::New(isolate_);
  tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate_, "CanvasGradient"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->PrototypeTemplate()->Set(
      isolate_, "addColorStop",
      v8::FunctionTemplate::New(isolate_, GradientAddColorStop));
  gradient_template_.Reset(isolate_, tpl);
  return tpl;
}

ScriptEngine* ScriptEngine::From(v8::Isolate* isolate) {
  return static_cast<ScriptEngine*>(isolate->GetData(0));
}

void ScriptEngine::PrintCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  PrintArgs(info);
}

void ScriptEngine::ConsoleLogCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  PrintArgs(info);
}

void ScriptEngine::LoadScriptCallback(
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

void ScriptEngine::SetTimeoutCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    ThrowTypeError(isolate, "setTimeout(callback, delay?) requires a function");
    return;
  }

  auto* engine = From(isolate);
  const std::uint32_t timer_id = engine->next_timer_id_++;
  auto context = isolate->GetCurrentContext();
  auto callback = info[0].As<v8::Function>();
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Value> result;
  if (!callback->Call(context, context->Global(), 0, nullptr).ToLocal(&result)) {
    ThrowError(isolate, FormatException(isolate, &try_catch));
    return;
  }

  info.GetReturnValue().Set(v8::Integer::NewFromUnsigned(isolate, timer_id));
}

void ScriptEngine::ClearTimeoutCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
}

void ScriptEngine::RequestAnimationFrameCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    ThrowTypeError(isolate,
                   "requestAnimationFrame(callback) requires a function");
    return;
  }

  auto* engine = From(isolate);
  const std::uint32_t timer_id = engine->next_timer_id_++;
  auto context = isolate->GetCurrentContext();
  auto callback = info[0].As<v8::Function>();
  v8::Local<v8::Value> argv[] = {v8::Number::New(isolate, NowMilliseconds())};
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Value> result;
  if (!callback->Call(context, context->Global(), 1, argv).ToLocal(&result)) {
    ThrowError(isolate, FormatException(isolate, &try_catch));
    return;
  }

  info.GetReturnValue().Set(v8::Integer::NewFromUnsigned(isolate, timer_id));
}

void ScriptEngine::CancelAnimationFrameCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
}

void ScriptEngine::PerformanceNowCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(v8::Number::New(info.GetIsolate(), NowMilliseconds()));
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

  const float pixel_ratio = GetDefaultDevicePixelRatio();
  const float exposed_pixel_ratio = GetExposedDevicePixelRatio();
  auto surface = CanvasSurface::CreateRaster(static_cast<int>(width),
                                             static_cast<int>(height),
                                             pixel_ratio);
  if (!surface) {
    ThrowError(isolate, "failed to create raster canvas surface");
    return;
  }

  auto* engine = From(isolate);
  engine->canvases_.push_back(std::make_unique<CanvasHandle>(surface));
  auto* handle = engine->canvases_.back().get();
  info.This()->SetAlignedPointerInInternalField(0, handle, kCanvasHandleTag);

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

  auto context_object = canvas->context_object.Get(isolate);
  context_object
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "canvas"),
            info.This())
      .Check();
  info.GetReturnValue().Set(context_object);
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

void ScriptEngine::CanvasWidthSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* canvas =
      Unwrap<CanvasHandle>(info.GetIsolate(), info.HolderV2(), kCanvasHandleTag);
  if (!canvas) {
    return;
  }

  double width = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &width) ||
      !canvas->surface->Resize(static_cast<int>(width), canvas->surface->height())) {
    ThrowTypeError(info.GetIsolate(), "width must be a positive number");
    return;
  }

  if (canvas->context_handle) {
    canvas->context_handle->context->ResetState();
  }
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

void ScriptEngine::CanvasHeightSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* canvas =
      Unwrap<CanvasHandle>(info.GetIsolate(), info.HolderV2(), kCanvasHandleTag);
  if (!canvas) {
    return;
  }

  double height = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &height) ||
      !canvas->surface->Resize(canvas->surface->width(), static_cast<int>(height))) {
    ThrowTypeError(info.GetIsolate(), "height must be a positive number");
    return;
  }

  if (canvas->context_handle) {
    canvas->context_handle->context->ResetState();
  }
}

void ScriptEngine::CanvasSetAttribute(
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

void ScriptEngine::CanvasAddEventListener(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
}

void ScriptEngine::CanvasRemoveEventListener(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
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
  if (value->IsString()) {
    if (!handle->context->SetFillStyle(ToStdString(info.GetIsolate(), value))) {
      ThrowTypeError(info.GetIsolate(), "fillStyle must be a valid CSS color");
    }
    return;
  }

  if (value->IsObject()) {
    auto object = value.As<v8::Object>();
    if (auto* gradient =
            Unwrap<GradientHandle>(info.GetIsolate(), object, kGradientHandleTag)) {
      handle->context->SetFillShader(gradient->BuildShader(), "[object CanvasGradient]");
      return;
    }
  }

  ThrowTypeError(info.GetIsolate(),
                 "fillStyle must be a CSS color or CanvasGradient");
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
  if (value->IsString()) {
    if (!handle->context->SetStrokeStyle(ToStdString(info.GetIsolate(), value))) {
      ThrowTypeError(info.GetIsolate(), "strokeStyle must be a valid CSS color");
    }
    return;
  }

  if (value->IsObject()) {
    auto object = value.As<v8::Object>();
    if (auto* gradient =
            Unwrap<GradientHandle>(info.GetIsolate(), object, kGradientHandleTag)) {
      handle->context->SetStrokeShader(gradient->BuildShader(),
                                       "[object CanvasGradient]");
      return;
    }
  }

  ThrowTypeError(info.GetIsolate(),
                 "strokeStyle must be a CSS color or CanvasGradient");
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

void ScriptEngine::FontGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(ToV8String(info.GetIsolate(), handle->context->font()));
}

void ScriptEngine::FontSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  if (!value->IsString() ||
      !handle->context->SetFont(ToStdString(info.GetIsolate(), value))) {
    ThrowTypeError(info.GetIsolate(), "font must contain a valid px size");
  }
}

void ScriptEngine::TextAlignGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(
      ToV8String(info.GetIsolate(), handle->context->text_align()));
}

void ScriptEngine::TextAlignSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  if (!value->IsString() ||
      !handle->context->SetTextAlign(ToStdString(info.GetIsolate(), value))) {
    ThrowTypeError(info.GetIsolate(), "textAlign must be a valid alignment");
  }
}

void ScriptEngine::TextBaselineGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(
      ToV8String(info.GetIsolate(), handle->context->text_baseline()));
}

void ScriptEngine::TextBaselineSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  if (!value->IsString() ||
      !handle->context->SetTextBaseline(ToStdString(info.GetIsolate(), value))) {
    ThrowTypeError(info.GetIsolate(),
                   "textBaseline must be a valid baseline value");
  }
}

void ScriptEngine::GlobalAlphaGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(handle->context->global_alpha());
}

void ScriptEngine::GlobalAlphaSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  double alpha = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &alpha) ||
      !handle->context->SetGlobalAlpha(static_cast<float>(alpha))) {
    ThrowTypeError(info.GetIsolate(), "globalAlpha must be numeric");
  }
}

void ScriptEngine::GlobalCompositeOperationGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(ToV8String(
      info.GetIsolate(), handle->context->global_composite_operation()));
}

void ScriptEngine::GlobalCompositeOperationSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  if (!value->IsString() ||
      !handle->context->SetGlobalCompositeOperation(
          ToStdString(info.GetIsolate(), value))) {
    ThrowTypeError(info.GetIsolate(),
                   "globalCompositeOperation must be supported");
  }
}

void ScriptEngine::LineCapGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(
      ToV8String(info.GetIsolate(), handle->context->line_cap()));
}

void ScriptEngine::LineCapSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  if (!value->IsString() ||
      !handle->context->SetLineCap(ToStdString(info.GetIsolate(), value))) {
    ThrowTypeError(info.GetIsolate(), "lineCap must be butt, round, or square");
  }
}

void ScriptEngine::LineJoinGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(
      ToV8String(info.GetIsolate(), handle->context->line_join()));
}

void ScriptEngine::LineJoinSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  if (!value->IsString() ||
      !handle->context->SetLineJoin(ToStdString(info.GetIsolate(), value))) {
    ThrowTypeError(info.GetIsolate(), "lineJoin must be miter, round, or bevel");
  }
}

void ScriptEngine::MiterLimitGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(handle->context->miter_limit());
}

void ScriptEngine::MiterLimitSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  double miter_limit = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &miter_limit) ||
      !handle->context->SetMiterLimit(static_cast<float>(miter_limit))) {
    ThrowTypeError(info.GetIsolate(), "miterLimit must be positive");
  }
}

void ScriptEngine::ShadowBlurGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(handle->context->shadow_blur());
}

void ScriptEngine::ShadowBlurSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  double shadow_blur = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &shadow_blur) ||
      !handle->context->SetShadowBlur(static_cast<float>(shadow_blur))) {
    ThrowTypeError(info.GetIsolate(), "shadowBlur must be non-negative");
  }
}

void ScriptEngine::ShadowColorGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(
      ToV8String(info.GetIsolate(), handle->context->shadow_color()));
}

void ScriptEngine::ShadowColorSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  if (!value->IsString() ||
      !handle->context->SetShadowColor(ToStdString(info.GetIsolate(), value))) {
    ThrowTypeError(info.GetIsolate(), "shadowColor must be a valid CSS color");
  }
}

void ScriptEngine::ShadowOffsetXGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(handle->context->shadow_offset_x());
}

void ScriptEngine::ShadowOffsetXSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  double shadow_offset_x = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &shadow_offset_x)) {
    ThrowTypeError(info.GetIsolate(), "shadowOffsetX must be numeric");
    return;
  }
  handle->context->SetShadowOffsetX(static_cast<float>(shadow_offset_x));
}

void ScriptEngine::ShadowOffsetYGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(handle->context->shadow_offset_y());
}

void ScriptEngine::ShadowOffsetYSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  double shadow_offset_y = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &shadow_offset_y)) {
    ThrowTypeError(info.GetIsolate(), "shadowOffsetY must be numeric");
    return;
  }
  handle->context->SetShadowOffsetY(static_cast<float>(shadow_offset_y));
}

void ScriptEngine::LineDashOffsetGetter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  info.GetReturnValue().Set(handle->context->line_dash_offset());
}

void ScriptEngine::LineDashOffsetSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = Unwrap<ContextHandle>(info.GetIsolate(), info.HolderV2(),
                                       kContextHandleTag);
  if (!handle) {
    return;
  }
  double dash_offset = 0.0;
  if (!ExtractDouble(info.GetIsolate(), value, &dash_offset)) {
    ThrowTypeError(info.GetIsolate(), "lineDashOffset must be numeric");
    return;
  }
  handle->context->SetLineDashOffset(static_cast<float>(dash_offset));
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

void ScriptEngine::CtxTransform(
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

void ScriptEngine::CtxSetTransform(
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

void ScriptEngine::CtxResetTransform(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->ResetTransform();
  }
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

void ScriptEngine::CtxBezierCurveTo(
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

void ScriptEngine::CtxRect(const v8::FunctionCallbackInfo<v8::Value>& info) {
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

void ScriptEngine::CtxClip(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (auto* handle =
          Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag)) {
    handle->context->Clip();
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

void ScriptEngine::CtxSetLineDash(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!handle || info.Length() < 1 || !info[0]->IsArray()) {
    ThrowTypeError(info.GetIsolate(), "setLineDash(segments) requires an array");
    return;
  }

  auto context = info.GetIsolate()->GetCurrentContext();
  auto array = info[0].As<v8::Array>();
  std::vector<float> segments;
  segments.reserve(array->Length());
  for (uint32_t i = 0; i < array->Length(); ++i) {
    v8::Local<v8::Value> value;
    if (!array->Get(context, i).ToLocal(&value)) {
      ThrowTypeError(info.GetIsolate(), "setLineDash segments must be numeric");
      return;
    }
    float segment = 0.0f;
    if (!ExtractFloat(info.GetIsolate(), value, &segment)) {
      ThrowTypeError(info.GetIsolate(), "setLineDash segments must be numeric");
      return;
    }
    segments.push_back(segment);
  }

  handle->context->SetLineDash(segments);
}

void ScriptEngine::CtxGetLineDash(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* handle =
      Unwrap<ContextHandle>(info.GetIsolate(), info.This(), kContextHandleTag);
  if (!handle) {
    return;
  }

  const auto& line_dash = handle->context->line_dash();
  auto array =
      v8::Array::New(info.GetIsolate(), static_cast<int>(line_dash.size()));
  auto context = info.GetIsolate()->GetCurrentContext();
  for (uint32_t i = 0; i < line_dash.size(); ++i) {
    array->Set(context, i, v8::Number::New(info.GetIsolate(), line_dash[i]))
        .Check();
  }
  info.GetReturnValue().Set(array);
}

void ScriptEngine::CtxCreateLinearGradient(
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

  engine->gradients_.push_back(
      std::make_unique<GradientHandle>(GradientHandle::Kind::kLinear));
  auto* gradient = engine->gradients_.back().get();
  gradient->x0 = x0;
  gradient->y0 = y0;
  gradient->x1 = x1;
  gradient->y1 = y1;

  auto context = info.GetIsolate()->GetCurrentContext();
  auto object = engine->GetGradientTemplate()
                    ->InstanceTemplate()
                    ->NewInstance(context)
                    .ToLocalChecked();
  object->SetAlignedPointerInInternalField(0, gradient, kGradientHandleTag);
  info.GetReturnValue().Set(object);
}

void ScriptEngine::CtxCreateRadialGradient(
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

  engine->gradients_.push_back(
      std::make_unique<GradientHandle>(GradientHandle::Kind::kRadial));
  auto* gradient = engine->gradients_.back().get();
  gradient->x0 = x0;
  gradient->y0 = y0;
  gradient->r0 = r0;
  gradient->x1 = x1;
  gradient->y1 = y1;
  gradient->r1 = r1;

  auto context = info.GetIsolate()->GetCurrentContext();
  auto object = engine->GetGradientTemplate()
                    ->InstanceTemplate()
                    ->NewInstance(context)
                    .ToLocalChecked();
  object->SetAlignedPointerInInternalField(0, gradient, kGradientHandleTag);
  info.GetReturnValue().Set(object);
}

void ScriptEngine::GradientAddColorStop(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto* gradient =
      Unwrap<GradientHandle>(info.GetIsolate(), info.This(), kGradientHandleTag);
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

  gradient->stops.push_back({std::clamp(offset, 0.0f, 1.0f), parsed.value()});
}

void ScriptEngine::CtxMeasureText(
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

void ScriptEngine::CtxFillText(
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

void ScriptEngine::CtxStrokeText(
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
