#pragma once

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

  static ScriptEngine* From(v8::Isolate* isolate);

  static void PrintCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void ConsoleLogCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasConstructor(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasGetContext(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasSaveToPng(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CanvasWidthGetter(v8::Local<v8::Name> property,
                                const v8::PropertyCallbackInfo<v8::Value>& info);
  static void CanvasHeightGetter(v8::Local<v8::Name> property,
                                 const v8::PropertyCallbackInfo<v8::Value>& info);

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

  static void CtxSave(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxRestore(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxTranslate(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxScale(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxRotate(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxClearRect(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxFillRect(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxStrokeRect(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxBeginPath(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxMoveTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxLineTo(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxArc(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxClosePath(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxFill(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void CtxStroke(const v8::FunctionCallbackInfo<v8::Value>& info);

  std::unique_ptr<v8::Platform> platform_;
  std::unique_ptr<v8::ArrayBuffer::Allocator> allocator_;
  v8::Isolate* isolate_ = nullptr;
  v8::Global<v8::Context> context_;
  v8::Global<v8::FunctionTemplate> canvas_template_;
  v8::Global<v8::FunctionTemplate> context_2d_template_;
  std::vector<std::unique_ptr<CanvasHandle>> canvases_;
};

}  // namespace canvas_engine
