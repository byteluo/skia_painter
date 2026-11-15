#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace canvas_engine {

class ScriptEngine {
 public:
  struct Options {
    float device_pixel_ratio = 3.0f;
    bool expose_gc = false;
    bool realtime_clock = false;
    std::optional<double> clock_epoch_ms;
    std::filesystem::path font_dir;
    bool allow_system_fonts = false;
  };

  ScriptEngine();
  explicit ScriptEngine(Options options);
  ~ScriptEngine();

  ScriptEngine(const ScriptEngine&) = delete;
  ScriptEngine& operator=(const ScriptEngine&) = delete;

  bool RunScriptFile(const std::string& script_path);
  bool RunScriptSource(const std::string& source,
                       const std::string& resource_name);
  const std::string& last_error() const;
  std::string last_result_json() const;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace canvas_engine
