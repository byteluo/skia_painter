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

std::string DoubleLiteral(double value) {
  std::ostringstream stream;
  stream << std::setprecision(17) << value;
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

bool EChartsRenderer::RenderBatchRequest(const std::string& request_json,
                                         float default_dpr,
                                         double default_settle_ms,
                                         std::string* response_json) {
  if (response_json == nullptr) {
    last_error_ = "batch response output must not be null";
    return false;
  }
  if (!Initialize()) {
    return false;
  }
  if (!(default_dpr > 0.0f) || !std::isfinite(default_dpr)) {
    last_error_ = "batch default DPR must be positive";
    return false;
  }
  if (default_settle_ms < 0.0 || !std::isfinite(default_settle_ms)) {
    last_error_ = "batch settle time must be non-negative";
    return false;
  }

  const std::string source =
      "((request, defaultDpr, defaultSettleMs) => {"
      "try {"
      "if (!request || typeof request !== 'object' || Array.isArray(request)) "
      "throw new Error('request must be an object');"
      "if (typeof request.out !== 'string' || request.out.length === 0) "
      "throw new Error('request.out must be a non-empty string');"
      "const width = request.width === undefined ? 960 : request.width;"
      "const height = request.height === undefined ? 540 : request.height;"
      "const dpr = request.dpr === undefined ? defaultDpr : request.dpr;"
      "const settleMs = defaultSettleMs;"
      "if (!Number.isInteger(width) || width <= 0) "
      "throw new Error('request.width must be a positive integer');"
      "if (!Number.isInteger(height) || height <= 0) "
      "throw new Error('request.height must be a positive integer');"
      "if (typeof dpr !== 'number' || !Number.isFinite(dpr) || dpr <= 0) "
      "throw new Error('request.dpr must be a positive number');"
      "let option;"
      "if (request.optionFile !== undefined) {"
      "if (typeof request.optionFile !== 'string' || "
      "request.optionFile.length === 0) "
      "throw new Error('request.optionFile must be a non-empty string');"
      "option = loadScript(request.optionFile);"
      "} else if (Object.prototype.hasOwnProperty.call(request, 'option')) {"
      "option = request.option;"
      "} else {"
      "throw new Error('request must include option or optionFile');"
      "}"
      "const theme = request.theme === undefined ? '' : request.theme;"
      "const background = request.background === undefined ? '' : "
      "request.background;"
      "if (typeof theme !== 'string') "
      "throw new Error('request.theme must be a string');"
      "if (typeof background !== 'string') "
      "throw new Error('request.background must be a string');"
      "__renderChart(option, {width, height, dpr, settleMs, theme, background}, "
      "request.out);"
      "return {ok: true, out: request.out};"
      "} catch (error) {"
      "return {ok: false, error: String(error && error.message || error)};"
      "}"
      "})(JSON.parse(" +
      QuoteJsString(request_json) + ")," + FloatLiteral(default_dpr) + "," +
      DoubleLiteral(default_settle_ms) + ");";
  if (!engine_.RunScriptSource(source, "<echarts-batch>")) {
    last_error_ = engine_.last_error();
    return false;
  }
  *response_json = engine_.last_result_json();
  last_error_.clear();
  return true;
}

bool EChartsRenderer::RenderExpression(
    const std::string& option_expression,
    const RenderOptions& options) {
  if (!Initialize()) {
    return false;
  }
  if (options.width <= 0 || options.height <= 0 ||
      !(options.dpr > 0.0f) || !std::isfinite(options.dpr) ||
      options.settle_ms < 0.0 || !std::isfinite(options.settle_ms) ||
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
                 << ",settleMs:" << DoubleLiteral(options.settle_ms)
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
