#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "v8.h"
#include "canvas_engine/canvas/FontManager.h"

#include "canvas_engine/runtime/V8Compat.h"

namespace canvas_engine {

class HandleRegistry;
class EventLoop;
namespace binding_internal {
struct ContextHandle;
}

class ScriptEngineImpl {
 public:
  struct Options {
    float device_pixel_ratio = 3.0f;
    bool expose_gc = false;
    bool realtime_clock = false;
    std::optional<double> clock_epoch_ms;
    std::filesystem::path font_dir;
    bool allow_system_fonts = false;
  };

  ScriptEngineImpl();
  explicit ScriptEngineImpl(Options options);
  ~ScriptEngineImpl();

  ScriptEngineImpl(const ScriptEngineImpl&) = delete;
  ScriptEngineImpl& operator=(const ScriptEngineImpl&) = delete;

  bool RunScriptFile(const std::string& script_path);
  bool RunScriptSource(const std::string& source,
                       const std::string& resource_name);
  const std::string& last_error() const { return last_error_; }
  std::string last_result_json() const;
  template <auto Getter, auto Setter>
  static void AccessorGetter(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Value>& info);
  template <auto Getter, auto Setter>
  static void AccessorSetter(
      v8::Local<v8::Name> property, v8::Local<v8::Value> value,
      const V8Compat::SetterInfo& info);

 private:
  struct CanvasHandle;
  struct ContextObjectFinalizerData;
  struct GradientHandle;
  struct HandleFinalizerData;
  struct ImageHandle;
  struct Path2DHandle;
  struct PatternHandle;
  v8::Local<v8::Context> GetContext();
  v8::Local<v8::FunctionTemplate> GetCanvasTemplate();
  v8::Local<v8::FunctionTemplate> GetContext2DTemplate();
  v8::Local<v8::FunctionTemplate> GetGradientTemplate();
  v8::Local<v8::FunctionTemplate> GetImageTemplate();
  v8::Local<v8::FunctionTemplate> GetPath2DTemplate();
  v8::Local<v8::FunctionTemplate> GetPatternTemplate();
  bool ExecuteScriptFile(const std::filesystem::path& script_path,
                         v8::Local<v8::Value>* out_result);
  bool ExecuteScriptSource(const std::string& source,
                           const std::filesystem::path& resource_path,
                           v8::Local<v8::Value>* out_result);
  bool DrainPendingTasks();
  std::uint32_t EnqueueTask(v8::Local<v8::Function> callback,
                            bool animation_frame,
                            double ready_at_ms,
                            const std::vector<v8::Local<v8::Value>>& arguments = {});
  std::uint32_t EnqueueNativeTask(std::function<void()> callback,
                                 double ready_at_ms);
  double NowMs() const;
  void RecordDrawOp();
  void CancelTask(std::uint32_t task_id);
  std::uint64_t NextHandleId();
  std::filesystem::path ResolveScriptPath(const std::string& script_path) const;

  static ScriptEngineImpl* From(v8::Isolate* isolate);
  void* LookupHandle(v8::Isolate* isolate, v8::Local<v8::Object> object,
                     v8::EmbedderDataTypeTag tag) const;
  template <typename T>
  static T* UnwrapHandle(v8::Isolate* isolate,
                         v8::Local<v8::Object> object,
                         v8::EmbedderDataTypeTag tag) {
    auto* engine = From(isolate);
    return engine
               ? static_cast<T*>(engine->LookupHandle(isolate, object, tag))
               : nullptr;
  }
  static void HandleWeakFinalizer(
      const v8::WeakCallbackInfo<HandleFinalizerData>& data);
  static void ContextObjectWeakFinalizer(
      const v8::WeakCallbackInfo<ContextObjectFinalizerData>& data);

  static void PrintCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void ConsoleLogCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void GcCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void LoadScriptCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void SetTimeoutCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void ClearTimeoutCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void RequestAnimationFrameCallback(
      const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CancelAnimationFrameCallback(
      const v8::FunctionCallbackInfo<v8::Value>& info);
  static void PerformanceNowCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void HandleCountsCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasConstructor(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasGetContext(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasSaveToPng(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasWidthGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void CanvasWidthSetter(v8::Local<v8::Name> property,
                                v8::Local<v8::Value> value,
                                const V8Compat::SetterInfo& info);
  static void CanvasHeightGetter(v8::Local<v8::Name> property,
                                 const v8::PropertyCallbackInfo<v8::Value>& info);
  static void CanvasHeightSetter(v8::Local<v8::Name> property,
                                 v8::Local<v8::Value> value,
                                 const V8Compat::SetterInfo& info);
  static void CanvasSetAttribute(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasAddEventListener(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasRemoveEventListener(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void ImageConstructor(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void ImageWidthGetter(v8::Local<v8::Name> property,
                               const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ImageHeightGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ImageSrcGetter(v8::Local<v8::Name> property,
                             const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ImageSrcSetter(v8::Local<v8::Name> property,
                             v8::Local<v8::Value> value,
                             const V8Compat::SetterInfo& info);
  static void ImageCompleteGetter(v8::Local<v8::Name> property,
                                  const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ImageOnloadGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ImageOnloadSetter(v8::Local<v8::Name> property,
                                v8::Local<v8::Value> value,
                                const V8Compat::SetterInfo& info);
  static void ImageOnerrorGetter(v8::Local<v8::Name> property,
                                 const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ImageOnerrorSetter(v8::Local<v8::Name> property,
                                 v8::Local<v8::Value> value,
                                 const V8Compat::SetterInfo& info);
  static void Path2DConstructor(
      const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Path2DMoveTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Path2DLineTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Path2DQuadraticCurveTo(
      const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Path2DBezierCurveTo(
      const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Path2DRect(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Path2DArc(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Path2DArcTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Path2DEllipse(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Path2DClosePath(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void Path2DAddPath(const v8::FunctionCallbackInfo<v8::Value>& info);

  static void CtxSave(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxRestore(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxTranslate(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxScale(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxRotate(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxTransform(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxSetTransform(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxResetTransform(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxClearRect(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxFillRect(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxStrokeRect(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxBeginPath(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxMoveTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxLineTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxQuadraticCurveTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxBezierCurveTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxRect(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxArc(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxArcTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxEllipse(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxClosePath(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxClip(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxFill(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxStroke(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxIsPointInPath(
      const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxDrawImage(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxSetLineDash(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxGetLineDash(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxCreateLinearGradient(
      const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxCreateRadialGradient(
      const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxCreatePattern(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxGetImageData(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxPutImageData(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxMeasureText(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxFillText(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxStrokeText(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void GradientAddColorStop(
      const v8::FunctionCallbackInfo<v8::Value>& info);

  Options options_;
  std::shared_ptr<FontManager> font_manager_;
  std::unique_ptr<v8::ArrayBuffer::Allocator> allocator_;
  v8::Isolate* isolate_ = nullptr;
  v8::Global<v8::Context> context_;
  v8::Global<v8::Value> last_result_;
  v8::Global<v8::FunctionTemplate> canvas_template_;
  v8::Global<v8::FunctionTemplate> context_2d_template_;
  v8::Global<v8::FunctionTemplate> gradient_template_;
  v8::Global<v8::FunctionTemplate> image_template_;
  v8::Global<v8::FunctionTemplate> path_2d_template_;
  v8::Global<v8::FunctionTemplate> pattern_template_;
  std::vector<std::filesystem::path> script_stack_;
  std::uint64_t next_handle_id_ = 1;
  std::chrono::steady_clock::time_point engine_start_time_ =
      std::chrono::steady_clock::now();
  double clock_epoch_ms_ = 0.0;
  std::string last_error_;
  std::unique_ptr<EventLoop> event_loop_;
  std::unique_ptr<HandleRegistry> handle_registry_;
  std::unordered_map<std::uint64_t, std::unique_ptr<CanvasHandle>> canvases_;
  std::unordered_map<std::uint64_t, std::unique_ptr<GradientHandle>> gradients_;
  std::unordered_map<std::uint64_t, std::unique_ptr<ImageHandle>> images_;
  std::unordered_map<std::uint64_t, std::unique_ptr<Path2DHandle>> path2ds_;
  std::unordered_map<std::uint64_t, std::unique_ptr<PatternHandle>> patterns_;
};

}  // namespace canvas_engine
