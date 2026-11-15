#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "libplatform/libplatform.h"
#include "v8.h"

#include "canvas_engine/runtime/V8Compat.h"

namespace canvas_engine {

class ScriptEngine {
 public:
  ScriptEngine();
  ~ScriptEngine();

  ScriptEngine(const ScriptEngine&) = delete;
  ScriptEngine& operator=(const ScriptEngine&) = delete;

  bool RunScriptFile(const std::string& script_path);

 private:
  struct CanvasHandle;
  struct ContextObjectFinalizerData;
  struct GradientHandle;
  struct HandleFinalizerData;
  struct ImageHandle;
  struct PatternHandle;
  struct PendingTask {
    std::uint32_t id = 0;
    bool animation_frame = false;
    bool cancelled = false;
    double ready_at_ms = 0.0;
    v8::Global<v8::Function> callback;
    std::vector<v8::Global<v8::Value>> arguments;
    std::function<void()> native;
  };

  v8::Local<v8::Context> GetContext();
  v8::Local<v8::FunctionTemplate> GetCanvasTemplate();
  v8::Local<v8::FunctionTemplate> GetContext2DTemplate();
  v8::Local<v8::FunctionTemplate> GetGradientTemplate();
  v8::Local<v8::FunctionTemplate> GetImageTemplate();
  v8::Local<v8::FunctionTemplate> GetPatternTemplate();
  bool ExecuteScriptFile(const std::filesystem::path& script_path,
                         v8::Local<v8::Value>* out_result);
  bool DrainPendingTasks();
  std::uint32_t EnqueueTask(v8::Local<v8::Function> callback,
                            bool animation_frame,
                            double ready_at_ms,
                            const std::vector<v8::Local<v8::Value>>& arguments = {});
  std::uint32_t EnqueueNativeTask(std::function<void()> callback,
                                 double ready_at_ms);
  double NowMs() const;
  void RecordDrawOp() { ++draw_op_count_; }
  void CancelTask(std::uint32_t task_id);
  std::uint64_t NextHandleId();
  std::filesystem::path ResolveScriptPath(const std::string& script_path) const;
  CanvasHandle* LiveCanvasHandle(CanvasHandle* handle) const;
  GradientHandle* LiveGradientHandle(GradientHandle* handle) const;
  ImageHandle* LiveImageHandle(ImageHandle* handle) const;
  PatternHandle* LivePatternHandle(PatternHandle* handle) const;

  static ScriptEngine* From(v8::Isolate* isolate);
  static void HandleWeakFinalizer(
      const v8::WeakCallbackInfo<HandleFinalizerData>& data);
  static void ContextObjectWeakFinalizer(
      const v8::WeakCallbackInfo<ContextObjectFinalizerData>& data);

  static void PrintCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void ConsoleLogCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
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

  static void FillStyleGetter(v8::Local<v8::Name> property,
                              const v8::PropertyCallbackInfo<v8::Value>& info);
  static void FillStyleSetter(v8::Local<v8::Name> property,
                              v8::Local<v8::Value> value,
                              const V8Compat::SetterInfo& info);
  static void StrokeStyleGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void StrokeStyleSetter(v8::Local<v8::Name> property,
                                v8::Local<v8::Value> value,
                                const V8Compat::SetterInfo& info);
  static void LineWidthGetter(v8::Local<v8::Name> property,
                              const v8::PropertyCallbackInfo<v8::Value>& info);
  static void LineWidthSetter(v8::Local<v8::Name> property,
                              v8::Local<v8::Value> value,
                              const V8Compat::SetterInfo& info);
  static void FontGetter(v8::Local<v8::Name> property,
                         const v8::PropertyCallbackInfo<v8::Value>& info);
  static void FontSetter(v8::Local<v8::Name> property,
                         v8::Local<v8::Value> value,
                         const V8Compat::SetterInfo& info);
  static void TextAlignGetter(v8::Local<v8::Name> property,
                              const v8::PropertyCallbackInfo<v8::Value>& info);
  static void TextAlignSetter(v8::Local<v8::Name> property,
                              v8::Local<v8::Value> value,
                              const V8Compat::SetterInfo& info);
  static void TextBaselineGetter(v8::Local<v8::Name> property,
                                 const v8::PropertyCallbackInfo<v8::Value>& info);
  static void TextBaselineSetter(v8::Local<v8::Name> property,
                                 v8::Local<v8::Value> value,
                                 const V8Compat::SetterInfo& info);
  static void GlobalAlphaGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void GlobalAlphaSetter(v8::Local<v8::Name> property,
                                v8::Local<v8::Value> value,
                                const V8Compat::SetterInfo& info);
  static void GlobalCompositeOperationGetter(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Value>& info);
  static void GlobalCompositeOperationSetter(
      v8::Local<v8::Name> property, v8::Local<v8::Value> value,
      const V8Compat::SetterInfo& info);
  static void LineCapGetter(v8::Local<v8::Name> property,
                            const v8::PropertyCallbackInfo<v8::Value>& info);
  static void LineCapSetter(v8::Local<v8::Name> property,
                            v8::Local<v8::Value> value,
                            const V8Compat::SetterInfo& info);
  static void LineJoinGetter(v8::Local<v8::Name> property,
                             const v8::PropertyCallbackInfo<v8::Value>& info);
  static void LineJoinSetter(v8::Local<v8::Name> property,
                             v8::Local<v8::Value> value,
                             const V8Compat::SetterInfo& info);
  static void MiterLimitGetter(v8::Local<v8::Name> property,
                               const v8::PropertyCallbackInfo<v8::Value>& info);
  static void MiterLimitSetter(v8::Local<v8::Name> property,
                               v8::Local<v8::Value> value,
                               const V8Compat::SetterInfo& info);
  static void ShadowBlurGetter(v8::Local<v8::Name> property,
                               const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ShadowBlurSetter(v8::Local<v8::Name> property,
                               v8::Local<v8::Value> value,
                               const V8Compat::SetterInfo& info);
  static void ShadowColorGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ShadowColorSetter(v8::Local<v8::Name> property,
                                v8::Local<v8::Value> value,
                                const V8Compat::SetterInfo& info);
  static void ShadowOffsetXGetter(v8::Local<v8::Name> property,
                                  const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ShadowOffsetXSetter(v8::Local<v8::Name> property,
                                  v8::Local<v8::Value> value,
                                  const V8Compat::SetterInfo& info);
  static void ShadowOffsetYGetter(v8::Local<v8::Name> property,
                                  const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ShadowOffsetYSetter(v8::Local<v8::Name> property,
                                  v8::Local<v8::Value> value,
                                  const V8Compat::SetterInfo& info);
  static void LineDashOffsetGetter(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Value>& info);
  static void LineDashOffsetSetter(
      v8::Local<v8::Name> property, v8::Local<v8::Value> value,
      const V8Compat::SetterInfo& info);

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

  std::unique_ptr<v8::Platform> platform_;
  std::unique_ptr<v8::ArrayBuffer::Allocator> allocator_;
  v8::Isolate* isolate_ = nullptr;
  v8::Global<v8::Context> context_;
  v8::Global<v8::FunctionTemplate> canvas_template_;
  v8::Global<v8::FunctionTemplate> context_2d_template_;
  v8::Global<v8::FunctionTemplate> gradient_template_;
  v8::Global<v8::FunctionTemplate> image_template_;
  v8::Global<v8::FunctionTemplate> pattern_template_;
  std::vector<std::filesystem::path> script_stack_;
  std::uint32_t next_timer_id_ = 1;
  std::uint64_t next_handle_id_ = 1;
  std::chrono::steady_clock::time_point engine_start_time_ =
      std::chrono::steady_clock::now();
  double clock_offset_ms_ = 0.0;
  std::uint64_t draw_op_count_ = 0;
  std::size_t consecutive_idle_frame_batches_ = 0;
  double next_frame_ms_ = 0.0;
  bool draining_tasks_ = false;
  std::string last_error_;
  std::unordered_map<std::uint64_t, std::unique_ptr<CanvasHandle>> canvases_;
  std::unordered_map<std::uint64_t, std::unique_ptr<GradientHandle>> gradients_;
  std::unordered_map<std::uint64_t, std::unique_ptr<ImageHandle>> images_;
  std::vector<PendingTask> pending_tasks_;
  std::unordered_map<std::uint64_t, std::unique_ptr<PatternHandle>> patterns_;
};

}  // namespace canvas_engine
