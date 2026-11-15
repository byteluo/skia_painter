#include "canvas_engine/echarts/EChartsRenderer.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>

#include "canvas_engine/runtime/ScriptEngine.h"

namespace canvas_engine {

std::string_view EChartsHostSource();

namespace {

std::string QuoteJsString(const std::string& value) {
  constexpr char kHex[] = "0123456789abcdef";
  std::string quoted = "\"";
  for (const unsigned char character : value) {
    switch (character) {
      case '"':
        quoted += "\\\"";
        break;
      case '\\':
        quoted += "\\\\";
        break;
      case '\b':
        quoted += "\\b";
        break;
      case '\f':
        quoted += "\\f";
        break;
      case '\n':
        quoted += "\\n";
        break;
      case '\r':
        quoted += "\\r";
        break;
      case '\t':
        quoted += "\\t";
        break;
      default:
        if (character < 0x20) {
          quoted += "\\u00";
          quoted += kHex[character >> 4];
          quoted += kHex[character & 0x0f];
        } else {
          quoted += static_cast<char>(character);
        }
    }
  }
  quoted += '"';
  return quoted;
}

std::string FloatLiteral(float value) {
  std::ostringstream stream;
  stream << std::setprecision(9) << value;
  return stream.str();
}

}  // namespace

EChartsRenderer::EChartsRenderer(ScriptEngine& engine,
                                 std::filesystem::path echarts_path)
    : engine_(engine), echarts_path_(std::move(echarts_path)) {}

bool EChartsRenderer::Initialize() {
  if (initialized_) {
    return true;
  }
  if (!std::filesystem::is_regular_file(echarts_path_)) {
    last_error_ = "ECharts library not found: " + echarts_path_.string();
    return false;
  }
  if (!engine_.RunScriptFile(echarts_path_.string())) {
    last_error_ = engine_.last_error();
    return false;
  }
  if (!engine_.RunScriptSource(std::string(EChartsHostSource()),
                              "<echarts-host>")) {
    last_error_ = engine_.last_error();
    return false;
  }
  initialized_ = true;
  last_error_.clear();
  return true;
}

bool EChartsRenderer::Render(const std::string& option_json,
                             const RenderOptions& options) {
  return RenderExpression("JSON.parse(" + QuoteJsString(option_json) + ")",
                          options);
}

bool EChartsRenderer::RenderLastScriptResult(
    const RenderOptions& options) {
  return RenderExpression("globalThis.__canvasEngineLastScriptResult",
                          options);
}

bool EChartsRenderer::RenderExpression(
    const std::string& option_expression,
    const RenderOptions& options) {
  if (!Initialize()) {
    return false;
  }
  if (options.width <= 0 || options.height <= 0 ||
      !(options.dpr > 0.0f) || !std::isfinite(options.dpr) ||
      options.output_path.empty()) {
    last_error_ =
        "render requires positive dimensions, DPR, and an output path";
    return false;
  }

  std::error_code error;
  const auto output_path =
      std::filesystem::absolute(options.output_path, error);
  if (error) {
    last_error_ = "failed to resolve output path: " +
                  options.output_path.string();
    return false;
  }

  std::ostringstream render_options;
  render_options << "{width:" << options.width << ",height:" << options.height
                 << ",dpr:" << FloatLiteral(options.dpr)
                 << ",theme:" << QuoteJsString(options.theme)
                 << ",background:" << QuoteJsString(options.background)
                 << "}";
  const std::string source =
      "__renderChart(" + option_expression + "," + render_options.str() +
      "," + QuoteJsString(output_path.string()) + ");";
  if (!engine_.RunScriptSource(source, "<echarts-render>")) {
    last_error_ = engine_.last_error();
    return false;
  }
  last_error_.clear();
  return true;
}

}  // namespace canvas_engine
