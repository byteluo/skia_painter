#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "libplatform/libplatform.h"
#include "v8.h"

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

  v8::Local<v8::Context> GetContext();
  v8::Local<v8::FunctionTemplate> GetCanvasTemplate();
  v8::Local<v8::FunctionTemplate> GetContext2DTemplate();
  bool ExecuteScriptFile(const std::filesystem::path& script_path,
                         v8::Local<v8::Value>* out_result);
  std::filesystem::path ResolveScriptPath(const std::string& script_path) const;

  static ScriptEngine* From(v8::Isolate* isolate);

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
  static void CanvasConstructor(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasGetContext(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasSaveToPng(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasWidthGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void CanvasWidthSetter(v8::Local<v8::Name> property,
                                v8::Local<v8::Value> value,
                                const v8::PropertyCallbackInfo<void>& info);
  static void CanvasHeightGetter(v8::Local<v8::Name> property,
                                 const v8::PropertyCallbackInfo<v8::Value>& info);
  static void CanvasHeightSetter(v8::Local<v8::Name> property,
                                 v8::Local<v8::Value> value,
                                 const v8::PropertyCallbackInfo<void>& info);
  static void CanvasSetAttribute(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasAddEventListener(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasRemoveEventListener(const v8::FunctionCallbackInfo<v8::Value>& info);

  static void FillStyleGetter(v8::Local<v8::Name> property,
                              const v8::PropertyCallbackInfo<v8::Value>& info);
  static void FillStyleSetter(v8::Local<v8::Name> property,
                              v8::Local<v8::Value> value,
                              const v8::PropertyCallbackInfo<void>& info);
  static void StrokeStyleGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void StrokeStyleSetter(v8::Local<v8::Name> property,
                                v8::Local<v8::Value> value,
                                const v8::PropertyCallbackInfo<void>& info);
  static void LineWidthGetter(v8::Local<v8::Name> property,
                              const v8::PropertyCallbackInfo<v8::Value>& info);
  static void LineWidthSetter(v8::Local<v8::Name> property,
                              v8::Local<v8::Value> value,
                              const v8::PropertyCallbackInfo<void>& info);
  static void FontGetter(v8::Local<v8::Name> property,
                         const v8::PropertyCallbackInfo<v8::Value>& info);
  static void FontSetter(v8::Local<v8::Name> property,
                         v8::Local<v8::Value> value,
                         const v8::PropertyCallbackInfo<void>& info);
  static void TextAlignGetter(v8::Local<v8::Name> property,
                              const v8::PropertyCallbackInfo<v8::Value>& info);
  static void TextAlignSetter(v8::Local<v8::Name> property,
                              v8::Local<v8::Value> value,
                              const v8::PropertyCallbackInfo<void>& info);
  static void TextBaselineGetter(v8::Local<v8::Name> property,
                                 const v8::PropertyCallbackInfo<v8::Value>& info);
  static void TextBaselineSetter(v8::Local<v8::Name> property,
                                 v8::Local<v8::Value> value,
                                 const v8::PropertyCallbackInfo<void>& info);
  static void GlobalAlphaGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void GlobalAlphaSetter(v8::Local<v8::Name> property,
                                v8::Local<v8::Value> value,
                                const v8::PropertyCallbackInfo<void>& info);
  static void GlobalCompositeOperationGetter(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Value>& info);
  static void GlobalCompositeOperationSetter(
      v8::Local<v8::Name> property, v8::Local<v8::Value> value,
      const v8::PropertyCallbackInfo<void>& info);
  static void LineCapGetter(v8::Local<v8::Name> property,
                            const v8::PropertyCallbackInfo<v8::Value>& info);
  static void LineCapSetter(v8::Local<v8::Name> property,
                            v8::Local<v8::Value> value,
                            const v8::PropertyCallbackInfo<void>& info);
  static void LineJoinGetter(v8::Local<v8::Name> property,
                             const v8::PropertyCallbackInfo<v8::Value>& info);
  static void LineJoinSetter(v8::Local<v8::Name> property,
                             v8::Local<v8::Value> value,
                             const v8::PropertyCallbackInfo<void>& info);
  static void MiterLimitGetter(v8::Local<v8::Name> property,
                               const v8::PropertyCallbackInfo<v8::Value>& info);
  static void MiterLimitSetter(v8::Local<v8::Name> property,
                               v8::Local<v8::Value> value,
                               const v8::PropertyCallbackInfo<void>& info);
  static void ShadowBlurGetter(v8::Local<v8::Name> property,
                               const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ShadowBlurSetter(v8::Local<v8::Name> property,
                               v8::Local<v8::Value> value,
                               const v8::PropertyCallbackInfo<void>& info);
  static void ShadowColorGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ShadowColorSetter(v8::Local<v8::Name> property,
                                v8::Local<v8::Value> value,
                                const v8::PropertyCallbackInfo<void>& info);
  static void ShadowOffsetXGetter(v8::Local<v8::Name> property,
                                  const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ShadowOffsetXSetter(v8::Local<v8::Name> property,
                                  v8::Local<v8::Value> value,
                                  const v8::PropertyCallbackInfo<void>& info);
  static void ShadowOffsetYGetter(v8::Local<v8::Name> property,
                                  const v8::PropertyCallbackInfo<v8::Value>& info);
  static void ShadowOffsetYSetter(v8::Local<v8::Name> property,
                                  v8::Local<v8::Value> value,
                                  const v8::PropertyCallbackInfo<void>& info);

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
  static void CtxBezierCurveTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxRect(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxArc(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxClosePath(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxClip(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxFill(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxStroke(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxMeasureText(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxFillText(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxStrokeText(const v8::FunctionCallbackInfo<v8::Value>& info);

  std::unique_ptr<v8::Platform> platform_;
  std::unique_ptr<v8::ArrayBuffer::Allocator> allocator_;
  v8::Isolate* isolate_ = nullptr;
  v8::Global<v8::Context> context_;
  v8::Global<v8::FunctionTemplate> canvas_template_;
  v8::Global<v8::FunctionTemplate> context_2d_template_;
  std::vector<std::filesystem::path> script_stack_;
  std::uint32_t next_timer_id_ = 1;
  std::string last_error_;
  std::vector<std::unique_ptr<CanvasHandle>> canvases_;
};

}  // namespace canvas_engine
