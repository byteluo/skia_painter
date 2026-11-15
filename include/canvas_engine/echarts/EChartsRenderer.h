#pragma once

#include <filesystem>
#include <string>

namespace canvas_engine {

class ScriptEngine;

struct RenderOptions {
  std::filesystem::path output_path;
  int width = 960;
  int height = 540;
  float dpr = 3.0f;
  std::string theme;
  std::string background;
};

class EChartsRenderer {
 public:
  EChartsRenderer(ScriptEngine& engine, std::filesystem::path echarts_path);

  bool Initialize();
  bool Render(const std::string& option_json,
              const RenderOptions& options);
  bool RenderLastScriptResult(const RenderOptions& options);
  const std::string& last_error() const { return last_error_; }

 private:
  bool RenderExpression(const std::string& option_expression,
                        const RenderOptions& options);

  ScriptEngine& engine_;
  std::filesystem::path echarts_path_;
  bool initialized_ = false;
  std::string last_error_;
};

}  // namespace canvas_engine
