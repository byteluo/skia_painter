#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../HandleRegistry.h"
#include "../ScriptEngineImpl.h"
#include "canvas_engine/canvas/Canvas2DContext.h"
#include "canvas_engine/canvas/CanvasSurface.h"
#include "canvas_engine/canvas/ColorParser.h"
#include "canvas_engine/canvas/ImageAsset.h"
#include "include/effects/SkGradient.h"

namespace canvas_engine {

namespace binding_internal {

struct ContextHandle {
  explicit ContextHandle(std::unique_ptr<Canvas2DContext> ctx)
      : context(std::move(ctx)) {}

  std::unique_ptr<Canvas2DContext> context;
};

constexpr v8::EmbedderDataTypeTag kCanvasHandleTag = 1;
constexpr v8::EmbedderDataTypeTag kContextHandleTag = 2;
constexpr v8::EmbedderDataTypeTag kGradientHandleTag = 3;
constexpr v8::EmbedderDataTypeTag kImageHandleTag = 4;
constexpr v8::EmbedderDataTypeTag kPatternHandleTag = 5;
constexpr v8::EmbedderDataTypeTag kPath2DHandleTag = 6;

constexpr char kVirtualDateBootstrap[] = R"js(
(() => {
  const NativeDate = Date;
  const performanceNow = performance.now;
  const epoch = __canvasEngineClockEpochMs;
  const now = () => epoch + performanceNow();
  const VirtualDate = new Proxy(NativeDate, {
    apply() {
      return new NativeDate(now()).toString();
    },
    construct(target, args, newTarget) {
      return Reflect.construct(
        target, args.length === 0 ? [now()] : args, newTarget);
    },
    get(target, property, receiver) {
      return property === "now" ? now : Reflect.get(target, property, receiver);
    }
  });
  Object.defineProperty(NativeDate.prototype, "constructor", {
    value: VirtualDate,
    writable: true,
    configurable: true
  });
  globalThis.Date = VirtualDate;
  delete globalThis.__canvasEngineClockEpochMs;
})();
)js";

template <typename T>
T* Unwrap(v8::Isolate* isolate, v8::Local<v8::Object> object,
          v8::EmbedderDataTypeTag tag) {
  return static_cast<T*>(
      object->GetAlignedPointerFromInternalField(isolate, 0, tag));
}

inline v8::Local<v8::String> ToV8String(v8::Isolate* isolate,
                                 const std::string& value) {
  return v8::String::NewFromUtf8(isolate, value.c_str()).ToLocalChecked();
}

inline std::string ToStdString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  v8::String::Utf8Value utf8(isolate, value);
  return *utf8 ? *utf8 : "";
}

inline bool StartsWithCaseInsensitive(std::string_view value,
                               std::string_view prefix) {
  if (value.size() < prefix.size()) {
    return false;
  }
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(value[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i]))) {
      return false;
    }
  }
  return true;
}

inline int Base64Digit(unsigned char value) {
  if (value >= 'A' && value <= 'Z') {
    return value - 'A';
  }
  if (value >= 'a' && value <= 'z') {
    return value - 'a' + 26;
  }
  if (value >= '0' && value <= '9') {
    return value - '0' + 52;
  }
  if (value == '+') {
    return 62;
  }
  if (value == '/') {
    return 63;
  }
  return -1;
}

inline int HexDigit(unsigned char value) {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }
  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }
  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }
  return -1;
}

inline bool DecodeDataUrl(std::string_view url, std::vector<std::uint8_t>* bytes) {
  if (!StartsWithCaseInsensitive(url, "data:")) {
    return false;
  }
  const std::size_t comma = url.find(',');
  if (comma == std::string_view::npos) {
    return false;
  }

  std::string_view metadata = url.substr(5, comma - 5);
  const bool base64 =
      metadata.size() >= 7 &&
      StartsWithCaseInsensitive(metadata.substr(metadata.size() - 7),
                                ";base64");
  if (base64) {
    metadata.remove_suffix(7);
  }

  const std::string_view payload = url.substr(comma + 1);
  bytes->clear();
  bytes->reserve(payload.size());
  if (base64) {
    if (payload.size() % 4 != 0) {
      return false;
    }
    for (std::size_t i = 0; i < payload.size(); i += 4) {
      const bool last_group = i + 4 == payload.size();
      const int first = Base64Digit(static_cast<unsigned char>(payload[i]));
      const int second =
          Base64Digit(static_cast<unsigned char>(payload[i + 1]));
      const char third_char = payload[i + 2];
      const char fourth_char = payload[i + 3];
      const int third = third_char == '='
                            ? 0
                            : Base64Digit(static_cast<unsigned char>(third_char));
      const int fourth =
          fourth_char == '='
              ? 0
              : Base64Digit(static_cast<unsigned char>(fourth_char));
      if (first < 0 || second < 0 || third < 0 || fourth < 0 ||
          (third_char == '=' && fourth_char != '=') ||
          ((!last_group) && (third_char == '=' || fourth_char == '='))) {
        return false;
      }
      bytes->push_back(static_cast<std::uint8_t>((first << 2) | (second >> 4)));
      if (third_char != '=') {
        bytes->push_back(static_cast<std::uint8_t>((second << 4) | (third >> 2)));
      }
      if (fourth_char != '=') {
        bytes->push_back(static_cast<std::uint8_t>((third << 6) | fourth));
      }
    }
    return true;
  }

  for (std::size_t i = 0; i < payload.size(); ++i) {
    if (payload[i] == '%') {
      if (i + 2 >= payload.size()) {
        return false;
      }
      const int high = HexDigit(static_cast<unsigned char>(payload[i + 1]));
      const int low = HexDigit(static_cast<unsigned char>(payload[i + 2]));
      if (high < 0 || low < 0) {
        return false;
      }
      bytes->push_back(static_cast<std::uint8_t>((high << 4) | low));
      i += 2;
    } else {
      bytes->push_back(static_cast<std::uint8_t>(payload[i]));
    }
  }
  return true;
}

inline bool ExtractDouble(const v8::FunctionCallbackInfo<v8::Value>& info, int index,
                   double* out_value) {
  if (info.Length() <= index) {
    return false;
  }
  return info[index]
      ->NumberValue(info.GetIsolate()->GetCurrentContext())
      .To(out_value);
}

inline bool ExtractDouble(v8::Isolate* isolate, v8::Local<v8::Value> value,
                   double* out_value) {
  return value->NumberValue(isolate->GetCurrentContext()).To(out_value);
}

inline bool ExtractFloat(v8::Isolate* isolate, v8::Local<v8::Value> value,
                  float* out_value) {
  double raw = 0.0;
  if (!ExtractDouble(isolate, value, &raw)) {
    return false;
  }
  *out_value = static_cast<float>(raw);
  return true;
}

inline void ThrowTypeError(v8::Isolate* isolate, const char* message) {
  isolate->ThrowException(
      v8::Exception::TypeError(ToV8String(isolate, message)));
}

inline void ThrowError(v8::Isolate* isolate, const std::string& message) {
  isolate->ThrowException(v8::Exception::Error(ToV8String(isolate, message)));
}

inline std::string FormatException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
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

inline void PrintArgs(const v8::FunctionCallbackInfo<v8::Value>& info) {
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

inline float GetDefaultDevicePixelRatio() {
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

inline float GetExposedDevicePixelRatio() {
  return 1.0f;
}

inline SkTileMode RepeatModeToTileMode(const std::string& repeat, bool for_x_axis) {
  if (repeat == "repeat-x") {
    return for_x_axis ? SkTileMode::kRepeat : SkTileMode::kClamp;
  }
  if (repeat == "repeat-y") {
    return for_x_axis ? SkTileMode::kClamp : SkTileMode::kRepeat;
  }
  if (repeat == "no-repeat") {
    return SkTileMode::kClamp;
  }
  return SkTileMode::kRepeat;
}

class GradientSource final : public PaintSource {
 public:
  enum class Kind {
    kLinear,
    kRadial,
  };

  struct ColorStop {
    float offset = 0.0f;
    SkColor color = SK_ColorBLACK;
  };

  GradientSource(Kind kind, float x0, float y0, float r0, float x1, float y1,
                 float r1)
      : kind_(kind),
        x0_(x0),
        y0_(y0),
        r0_(r0),
        x1_(x1),
        y1_(y1),
        r1_(r1) {}

  sk_sp<SkShader> shader() const override {
    if (cache_valid_) {
      return cached_shader_;
    }
    cache_valid_ = true;
    if (stops_.empty()) {
      cached_shader_.reset();
      return cached_shader_;
    }

    std::vector<ColorStop> sorted = stops_;
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
    if (kind_ == Kind::kLinear) {
      const SkPoint points[2] = {SkPoint::Make(x0_, y0_),
                                 SkPoint::Make(x1_, y1_)};
      cached_shader_ = SkShaders::LinearGradient(points, gradient);
    } else {
      cached_shader_ = SkShaders::TwoPointConicalGradient(
          SkPoint::Make(x0_, y0_), r0_, SkPoint::Make(x1_, y1_), r1_,
          gradient);
    }
    return cached_shader_;
  }

  const std::string& description() const override { return description_; }

  void AddColorStop(float offset, SkColor color) {
    stops_.push_back({offset, color});
    cache_valid_ = false;
    cached_shader_.reset();
  }

 private:
  Kind kind_;
  float x0_ = 0.0f;
  float y0_ = 0.0f;
  float r0_ = 0.0f;
  float x1_ = 0.0f;
  float y1_ = 0.0f;
  float r1_ = 0.0f;
  std::vector<ColorStop> stops_;
  mutable bool cache_valid_ = false;
  mutable sk_sp<SkShader> cached_shader_;
  const std::string description_ = "[object CanvasGradient]";
};

class FixedShaderSource final : public PaintSource {
 public:
  FixedShaderSource(sk_sp<SkShader> shader, std::string description)
      : shader_(std::move(shader)), description_(std::move(description)) {}

  sk_sp<SkShader> shader() const override { return shader_; }
  const std::string& description() const override { return description_; }

 private:
  sk_sp<SkShader> shader_;
  std::string description_;
};

inline bool ExtractSixNumbers(const v8::FunctionCallbackInfo<v8::Value>& info,
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

}  // namespace binding_internal

struct ScriptEngineImpl::HandleFinalizerData {
  enum class Kind {
    kCanvas,
    kGradient,
    kImage,
    kPath2D,
    kPattern,
  };

  ScriptEngineImpl* engine = nullptr;
  Kind kind = Kind::kCanvas;
  std::uint64_t id = 0;
};

struct ScriptEngineImpl::ContextObjectFinalizerData {
  ScriptEngineImpl* engine = nullptr;
  std::uint64_t canvas_id = 0;
};

struct ScriptEngineImpl::CanvasHandle {
  CanvasHandle(std::uint64_t handle_id,
               std::shared_ptr<CanvasSurface> surface_value)
      : header{static_cast<std::uint32_t>(binding_internal::kCanvasHandleTag), handle_id},
        id(handle_id),
        surface(std::move(surface_value)) {}

  HandleRegistry::Header header;
  std::uint64_t id = 0;
  std::shared_ptr<CanvasSurface> surface;
  std::unique_ptr<binding_internal::ContextHandle> context_handle;
  v8::Global<v8::Object> object;
  v8::Global<v8::Object> context_object;
  std::unique_ptr<HandleFinalizerData> finalizer_data;
  std::unique_ptr<ContextObjectFinalizerData> context_finalizer_data;
};

struct ScriptEngineImpl::ImageHandle {
  ImageHandle(std::uint64_t handle_id, v8::Isolate* isolate_value)
      : header{static_cast<std::uint32_t>(binding_internal::kImageHandleTag), handle_id},
        id(handle_id),
        isolate(isolate_value) {}

  void ResetCallbacks() {
    onload.Reset();
    onerror.Reset();
  }

  HandleRegistry::Header header;
  std::uint64_t id = 0;
  v8::Isolate* isolate = nullptr;
  std::string src;
  std::shared_ptr<ImageAsset> asset;
  bool complete = false;
  bool failed = false;
  std::uint64_t source_generation = 0;
  v8::Global<v8::Object> object;
  v8::Global<v8::Function> onload;
  v8::Global<v8::Function> onerror;
  std::unique_ptr<HandleFinalizerData> finalizer_data;
};

struct ScriptEngineImpl::PatternHandle {
  explicit PatternHandle(std::uint64_t handle_id)
      : header{static_cast<std::uint32_t>(binding_internal::kPatternHandleTag), handle_id},
        id(handle_id) {}

  HandleRegistry::Header header;
  std::uint64_t id = 0;
  v8::Global<v8::Object> object;
  std::unique_ptr<HandleFinalizerData> finalizer_data;
  std::shared_ptr<const PaintSource> source;
};

struct ScriptEngineImpl::Path2DHandle {
  Path2DHandle(std::uint64_t handle_id, SkPathBuilder path_value = {})
      : header{static_cast<std::uint32_t>(binding_internal::kPath2DHandleTag),
               handle_id},
        id(handle_id),
        path(std::move(path_value)) {}

  HandleRegistry::Header header;
  std::uint64_t id = 0;
  SkPathBuilder path;
  v8::Global<v8::Object> object;
  std::unique_ptr<HandleFinalizerData> finalizer_data;
};

struct ScriptEngineImpl::GradientHandle {
  GradientHandle(std::uint64_t handle_id,
                 std::shared_ptr<binding_internal::GradientSource> gradient_source)
      : header{static_cast<std::uint32_t>(binding_internal::kGradientHandleTag), handle_id},
        id(handle_id),
        source(std::move(gradient_source)) {}

  HandleRegistry::Header header;
  std::uint64_t id = 0;
  v8::Global<v8::Object> object;
  std::unique_ptr<HandleFinalizerData> finalizer_data;
  std::shared_ptr<binding_internal::GradientSource> source;
};

namespace binding_internal {

template <typename T>
struct AccessorSetterTraits;

template <typename Result, typename Class, typename Argument>
struct AccessorSetterTraits<Result (Class::*)(Argument)> {
  using argument_type = Argument;
};

struct AccessorBinding {
  const char* name;
  void (*getter)(v8::Local<v8::Name>,
                 const v8::PropertyCallbackInfo<v8::Value>&);
  void (*setter)(v8::Local<v8::Name>, v8::Local<v8::Value>,
                 const v8::PropertyCallbackInfo<void>&);
};

struct MethodBinding {
  const char* name;
  v8::FunctionCallback callback;
  int length;
};

template <auto Getter, auto Setter>
AccessorBinding BindAccessor(const char* name) {
  return {name, &ScriptEngineImpl::AccessorGetter<Getter, Setter>,
          &ScriptEngineImpl::AccessorSetter<Getter, Setter>};
}

inline void BindMethods(v8::Isolate* isolate,
                        v8::Local<v8::ObjectTemplate> prototype,
                        std::initializer_list<MethodBinding> methods) {
  for (const auto& method : methods) {
    prototype->Set(
        isolate, method.name,
        v8::FunctionTemplate::New(isolate, method.callback,
                                  v8::Local<v8::Value>(),
                                  v8::Local<v8::Signature>(), method.length));
  }
}

}

template <auto Getter, auto Setter>
void ScriptEngineImpl::AccessorGetter(
    v8::Local<v8::Name>,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto* handle = binding_internal::Unwrap<binding_internal::ContextHandle>(
      info.GetIsolate(), info.HolderV2(), binding_internal::kContextHandleTag);
  if (!handle) {
    return;
  }

  using Result = std::remove_cvref_t<
      std::invoke_result_t<decltype(Getter), Canvas2DContext&>>;
  if constexpr (std::is_same_v<Result, std::string>) {
    info.GetReturnValue().Set(binding_internal::ToV8String(
        info.GetIsolate(), std::invoke(Getter, *handle->context)));
  } else {
    info.GetReturnValue().Set(std::invoke(Getter, *handle->context));
  }
}

template <auto Getter, auto Setter>
void ScriptEngineImpl::AccessorSetter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<void>& info) {
  auto* handle = binding_internal::Unwrap<binding_internal::ContextHandle>(
      info.GetIsolate(), info.HolderV2(), binding_internal::kContextHandleTag);
  if (!handle) {
    return;
  }

  using Argument = typename binding_internal::AccessorSetterTraits<
      decltype(Setter)>::argument_type;
  if constexpr (std::is_same_v<Argument, std::string_view>) {
    if (!value->IsString()) {
      const std::string property_name =
          binding_internal::ToStdString(info.GetIsolate(), property);
      const bool fill_style = property_name == "fillStyle";
      const bool stroke_style = property_name == "strokeStyle";
      if ((fill_style || stroke_style) && value->IsObject()) {
        auto object = value.As<v8::Object>();
        if (auto* gradient = UnwrapHandle<GradientHandle>(
                info.GetIsolate(), object, binding_internal::kGradientHandleTag)) {
          if (fill_style) {
            handle->context->SetFillSource(gradient->source);
          } else {
            handle->context->SetStrokeSource(gradient->source);
          }
          return;
        }
        if (auto* pattern = UnwrapHandle<PatternHandle>(
                info.GetIsolate(), object, binding_internal::kPatternHandleTag)) {
          if (fill_style) {
            handle->context->SetFillSource(pattern->source);
          } else {
            handle->context->SetStrokeSource(pattern->source);
          }
        }
      }
      return;
    }
    const std::string converted = binding_internal::ToStdString(
        info.GetIsolate(), value);
    std::invoke(Setter, *handle->context, std::string_view(converted));
  } else if constexpr (std::is_floating_point_v<Argument>) {
    double number = 0.0;
    if (!binding_internal::ExtractDouble(info.GetIsolate(), value, &number) ||
        !std::isfinite(number)) {
      return;
    }
    const Argument converted = static_cast<Argument>(number);
    if (!std::isfinite(static_cast<double>(converted))) {
      return;
    }
    std::invoke(Setter, *handle->context, converted);
  } else if constexpr (std::is_same_v<Argument, bool>) {
    std::invoke(Setter, *handle->context, value->BooleanValue(info.GetIsolate()));
  }
}

}  // namespace canvas_engine
