#include "canvas_engine/runtime/ScriptEngine.h"

#include "ScriptEngineImpl.h"

namespace canvas_engine {

namespace {

ScriptEngineImpl::Options ToImplOptions(const ScriptEngine::Options& options) {
  ScriptEngineImpl::Options impl_options;
  impl_options.device_pixel_ratio = options.device_pixel_ratio;
  impl_options.expose_gc = options.expose_gc;
  impl_options.realtime_clock = options.realtime_clock;
  impl_options.clock_epoch_ms = options.clock_epoch_ms;
  impl_options.font_dir = options.font_dir;
  impl_options.allow_system_fonts = options.allow_system_fonts;
  return impl_options;
}

}  // namespace

class ScriptEngine::Impl {
 public:
  Impl() = default;
  explicit Impl(const Options& options) : engine(ToImplOptions(options)) {}

  ScriptEngineImpl engine;
};

ScriptEngine::ScriptEngine() : impl_(std::make_unique<Impl>()) {}

ScriptEngine::ScriptEngine(Options options)
    : impl_(std::make_unique<Impl>(options)) {}

ScriptEngine::~ScriptEngine() = default;

bool ScriptEngine::RunScriptFile(const std::string& script_path) {
  return impl_->engine.RunScriptFile(script_path);
}

bool ScriptEngine::RunScriptSource(const std::string& source,
                                   const std::string& resource_name) {
  return impl_->engine.RunScriptSource(source, resource_name);
}

const std::string& ScriptEngine::last_error() const {
  return impl_->engine.last_error();
}

std::string ScriptEngine::last_result_json() const {
  return impl_->engine.last_result_json();
}

}  // namespace canvas_engine
