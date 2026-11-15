#include "Cli.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>

#include "canvas_engine/echarts/EChartsRenderer.h"
#include "canvas_engine/runtime/ScriptEngine.h"

namespace {

constexpr char kUsage[] =
    "Usage:\n"
    "  canvas_engine [--expose-gc] <script.js>\n"
    "  canvas_engine run [--expose-gc] [--realtime-clock] "
    "[--epoch-ms n] <script.js>\n"
    "  canvas_engine render <option.json|option.js|-> -o <out.png> "
    "[--width 960] [--height 540] [--dpr 3] [--theme name] "
    "[--echarts path] [--realtime-clock] [--epoch-ms n] [--settle-ms n] "
    "[--animate] [--png-compression fast|default|small]\n"
    "  canvas_engine batch [--echarts path] [--realtime-clock] "
    "[--epoch-ms n] [--settle-ms n] [--animate] "
    "[--png-compression fast|default|small]\n"
    "  canvas_engine --help";

struct RenderArguments {
  std::string input;
  std::filesystem::path output;
  int width = 960;
  int height = 540;
  float dpr = 3.0f;
  std::string theme;
  std::optional<std::string> echarts_path;
  bool realtime_clock = false;
  bool animation = false;
  canvas_engine::PngCompression png_compression =
      canvas_engine::PngCompression::kDefault;
  std::optional<double> epoch_ms;
  double settle_ms = 3000.0;
  bool show_help = false;
};

void PrintUsage(std::ostream& stream) {
  stream << kUsage << std::endl;
}

float DefaultDevicePixelRatio() {
  const char* value = std::getenv("CANVAS_ENGINE_DPR");
  if (value == nullptr || *value == '\0') {
    return 3.0f;
  }
  try {
    const float ratio = std::stof(value);
    if (ratio > 0.0f && std::isfinite(ratio)) {
      return ratio;
    }
  } catch (...) {
  }
  return 3.0f;
}

canvas_engine::ScriptEngine::Options MakeEngineOptions(
    bool expose_gc,
    bool realtime_clock,
    std::optional<double> epoch_ms) {
  canvas_engine::ScriptEngine::Options options;
  options.device_pixel_ratio = DefaultDevicePixelRatio();
  options.expose_gc = expose_gc;
  options.realtime_clock = realtime_clock;
  options.clock_epoch_ms = epoch_ms;
  return options;
}

bool ParseFiniteDouble(const std::string& value, double* result) {
  try {
    std::size_t parsed = 0;
    const double number = std::stod(value, &parsed);
    if (parsed != value.size() || !std::isfinite(number)) {
      return false;
    }
    *result = number;
    return true;
  } catch (...) {
    return false;
  }
}

bool ParsePngCompression(const std::string& value,
                         canvas_engine::PngCompression* result) {
  if (value == "fast") {
    *result = canvas_engine::PngCompression::kFast;
    return true;
  }
  if (value == "default") {
    *result = canvas_engine::PngCompression::kDefault;
    return true;
  }
  if (value == "small") {
    *result = canvas_engine::PngCompression::kSmall;
    return true;
  }
  return false;
}

bool ReadEnvironmentEpoch(std::optional<double>* epoch_ms,
                          std::string* error_message) {
  const char* value = std::getenv("CANVAS_ENGINE_EPOCH_MS");
  if (value == nullptr || *value == '\0') {
    return true;
  }
  double parsed = 0.0;
  if (!ParseFiniteDouble(value, &parsed)) {
    *error_message = "CANVAS_ENGINE_EPOCH_MS must be a finite number";
    return false;
  }
  *epoch_ms = parsed;
  return true;
}

bool ParsePositiveInteger(const std::string& value, int* result) {
  try {
    std::size_t parsed = 0;
    const int number = std::stoi(value, &parsed);
    if (parsed != value.size() || number <= 0) {
      return false;
    }
    *result = number;
    return true;
  } catch (...) {
    return false;
  }
}

bool ParsePositiveFloat(const std::string& value, float* result) {
  try {
    std::size_t parsed = 0;
    const float number = std::stof(value, &parsed);
    if (parsed != value.size() || !(number > 0.0f) ||
        !std::isfinite(number)) {
      return false;
    }
    *result = number;
    return true;
  } catch (...) {
    return false;
  }
}

std::filesystem::path AbsolutePath(const std::filesystem::path& path) {
  std::error_code error;
  const auto absolute = std::filesystem::absolute(path, error);
  return error ? path : absolute;
}

std::filesystem::path ResolveExecutablePath(const std::string& argv0) {
  const std::filesystem::path supplied(argv0);
  std::error_code error;
  if (std::filesystem::is_regular_file(supplied, error)) {
    return AbsolutePath(supplied);
  }
  if (supplied.is_absolute() || supplied.has_parent_path()) {
    return AbsolutePath(supplied);
  }

  const char* path_value = std::getenv("PATH");
  if (path_value != nullptr) {
    const std::string search_path(path_value);
    std::size_t start = 0;
    while (start <= search_path.size()) {
      const std::size_t separator = search_path.find(':', start);
      const std::string directory =
          search_path.substr(start, separator == std::string::npos
                                        ? std::string::npos
                                        : separator - start);
      const auto candidate =
          (directory.empty() ? std::filesystem::current_path()
                             : std::filesystem::path(directory)) /
          supplied;
      error.clear();
      if (std::filesystem::is_regular_file(candidate, error)) {
        return AbsolutePath(candidate);
      }
      if (separator == std::string::npos) {
        break;
      }
      start = separator + 1;
    }
  }
  return AbsolutePath(supplied);
}

bool IsRegularFile(const std::filesystem::path& path) {
  std::error_code error;
  return std::filesystem::is_regular_file(path, error) && !error;
}

bool ResolveEChartsPath(const std::optional<std::string>& explicit_path,
                        const std::string& argv0,
                        std::filesystem::path* result,
                        std::string* error_message) {
  if (explicit_path.has_value()) {
    const std::filesystem::path path(*explicit_path);
    if (!IsRegularFile(path)) {
      *error_message =
          "ECharts library specified with --echarts was not found: " +
          path.string();
      return false;
    }
    *result = path;
    return true;
  }

  const char* environment_path = std::getenv("CANVAS_ENGINE_ECHARTS");
  if (environment_path != nullptr && *environment_path != '\0') {
    const std::filesystem::path path(environment_path);
    if (!IsRegularFile(path)) {
      *error_message =
          "ECharts library specified by CANVAS_ENGINE_ECHARTS was not found: " +
          path.string();
      return false;
    }
    *result = path;
    return true;
  }

  const std::filesystem::path local_path =
      "node_modules/echarts/dist/echarts.js";
  if (IsRegularFile(local_path)) {
    *result = local_path;
    return true;
  }

  const auto executable_directory = ResolveExecutablePath(argv0).parent_path();
  const auto executable_relative_path =
      executable_directory / ".." / "node_modules" / "echarts" / "dist" /
      "echarts.js";
  if (IsRegularFile(executable_relative_path)) {
    *result = executable_relative_path;
    return true;
  }

  *error_message =
      "ECharts library not found. Use --echarts <path>, set "
      "CANVAS_ENGINE_ECHARTS, or install node_modules/echarts.";
  return false;
}

bool ParseRenderArguments(int argc,
                          char** argv,
                          RenderArguments* arguments,
                          std::string* error_message) {
  arguments->dpr = DefaultDevicePixelRatio();
  for (int index = 2; index < argc; ++index) {
    const std::string argument(argv[index]);
    if (argument == "-o" || argument == "--width" ||
        argument == "--height" || argument == "--dpr" ||
        argument == "--theme" || argument == "--echarts" ||
        argument == "--epoch-ms" || argument == "--settle-ms" ||
        argument == "--png-compression") {
      if (index + 1 >= argc) {
        *error_message = "missing value for " + argument;
        return false;
      }
      const std::string value(argv[++index]);
      if (argument == "-o") {
        arguments->output = value;
      } else if (argument == "--width") {
        if (!ParsePositiveInteger(value, &arguments->width)) {
          *error_message = "--width must be a positive integer";
          return false;
        }
      } else if (argument == "--height") {
        if (!ParsePositiveInteger(value, &arguments->height)) {
          *error_message = "--height must be a positive integer";
          return false;
        }
      } else if (argument == "--dpr") {
        if (!ParsePositiveFloat(value, &arguments->dpr)) {
          *error_message = "--dpr must be a positive number";
          return false;
        }
      } else if (argument == "--theme") {
        arguments->theme = value;
      } else if (argument == "--echarts") {
        arguments->echarts_path = value;
      } else if (argument == "--settle-ms") {
        if (!ParseFiniteDouble(value, &arguments->settle_ms) ||
            arguments->settle_ms < 0.0) {
          *error_message = "--settle-ms must be a non-negative number";
          return false;
        }
      } else if (argument == "--png-compression") {
        if (!ParsePngCompression(value, &arguments->png_compression)) {
          *error_message =
              "--png-compression must be fast, default, or small";
          return false;
        }
      } else {
        double epoch_ms = 0.0;
        if (!ParseFiniteDouble(value, &epoch_ms)) {
          *error_message = "--epoch-ms must be a finite number";
          return false;
        }
        arguments->epoch_ms = epoch_ms;
      }
    } else if (argument == "--realtime-clock") {
      arguments->realtime_clock = true;
    } else if (argument == "--animate") {
      arguments->animation = true;
    } else if (argument == "--help") {
      arguments->show_help = true;
      return true;
    } else if (argument.size() > 1 && argument[0] == '-') {
      *error_message = "unknown render option: " + argument;
      return false;
    } else if (arguments->input.empty()) {
      arguments->input = argument;
    } else {
      *error_message = "render accepts exactly one option input";
      return false;
    }
  }

  if (arguments->input.empty()) {
    *error_message = "render requires an option input";
    return false;
  }
  if (arguments->output.empty()) {
    *error_message = "render requires -o <out.png>";
    return false;
  }
  if (!arguments->epoch_ms.has_value() &&
      !ReadEnvironmentEpoch(&arguments->epoch_ms, error_message)) {
    return false;
  }
  return true;
}

int RunScript(const std::string& script_path,
              bool expose_gc,
              bool realtime_clock,
              std::optional<double> epoch_ms) {
  canvas_engine::ScriptEngine engine(
      MakeEngineOptions(expose_gc, realtime_clock, epoch_ms));
  if (!engine.RunScriptFile(script_path)) {
    std::cerr << engine.last_error() << std::endl;
    return 1;
  }
  return 0;
}

int RunCommand(int argc, char** argv, int first_argument) {
  if (first_argument < argc &&
      std::string(argv[first_argument]) == "--help") {
    PrintUsage(std::cout);
    return 0;
  }
  bool expose_gc = false;
  bool realtime_clock = false;
  std::optional<double> epoch_ms;
  std::string error_message;
  std::string script_path;
  for (int index = first_argument; index < argc; ++index) {
    const std::string argument(argv[index]);
    if (argument == "--expose-gc") {
      expose_gc = true;
    } else if (argument == "--realtime-clock") {
      realtime_clock = true;
    } else if (argument == "--epoch-ms") {
      double parsed = 0.0;
      if (index + 1 >= argc ||
          !ParseFiniteDouble(argv[++index], &parsed)) {
        std::cerr << "--epoch-ms requires a finite number" << std::endl;
        return 2;
      }
      epoch_ms = parsed;
    } else if (argument == "--help") {
      PrintUsage(std::cout);
      return 0;
    } else if (!argument.empty() && argument[0] == '-') {
      std::cerr << "unknown run option: " << argument << std::endl;
      PrintUsage(std::cerr);
      return 2;
    } else if (script_path.empty()) {
      script_path = argument;
    } else {
      std::cerr << "run accepts exactly one script" << std::endl;
      PrintUsage(std::cerr);
      return 2;
    }
  }
  if (script_path.empty()) {
    PrintUsage(std::cerr);
    return 2;
  }
  if (!epoch_ms.has_value() &&
      !ReadEnvironmentEpoch(&epoch_ms, &error_message)) {
    std::cerr << error_message << std::endl;
    return 2;
  }
  return RunScript(script_path, expose_gc, realtime_clock, epoch_ms);
}

bool ReadOptionInput(const std::string& input_path,
                     std::string* contents,
                     std::string* error_message) {
  if (input_path == "-") {
    std::ostringstream stream;
    stream << std::cin.rdbuf();
    *contents = stream.str();
    return true;
  }

  std::ifstream input(input_path, std::ios::binary);
  if (!input) {
    *error_message = "failed to read option file: " + input_path;
    return false;
  }
  std::ostringstream stream;
  stream << input.rdbuf();
  *contents = stream.str();
  return true;
}

int RunRenderCommand(int argc, char** argv) {
  RenderArguments arguments;
  std::string error_message;
  if (!ParseRenderArguments(argc, argv, &arguments, &error_message)) {
    std::cerr << error_message << std::endl;
    PrintUsage(std::cerr);
    return 2;
  }
  if (arguments.show_help) {
    PrintUsage(std::cout);
    return 0;
  }

  std::filesystem::path echarts_path;
  if (!ResolveEChartsPath(arguments.echarts_path, argv[0], &echarts_path,
                          &error_message)) {
    std::cerr << error_message << std::endl;
    return 1;
  }

  canvas_engine::ScriptEngine engine(MakeEngineOptions(
      false, arguments.realtime_clock, arguments.epoch_ms));
  canvas_engine::EChartsRenderer renderer(engine, echarts_path);
  if (!renderer.Initialize()) {
    std::cerr << renderer.last_error() << std::endl;
    return 1;
  }

  canvas_engine::RenderOptions options;
  options.output_path = arguments.output;
  options.width = arguments.width;
  options.height = arguments.height;
  options.dpr = arguments.dpr;
  options.animation = arguments.animation;
  options.png_compression = arguments.png_compression;
  options.settle_ms = arguments.settle_ms;
  options.theme = arguments.theme;

  if (std::filesystem::path(arguments.input).extension() == ".js") {
    if (!engine.RunScriptFile(arguments.input)) {
      std::cerr << engine.last_error() << std::endl;
      return 1;
    }
    if (!renderer.RenderLastScriptResult(options)) {
      std::cerr << renderer.last_error() << std::endl;
      return 1;
    }
    return 0;
  }

  std::string option_json;
  if (!ReadOptionInput(arguments.input, &option_json, &error_message)) {
    std::cerr << error_message << std::endl;
    return 1;
  }
  if (!renderer.Render(option_json, options)) {
    std::cerr << renderer.last_error() << std::endl;
    return 1;
  }
  return 0;
}

std::string QuoteJsonString(const std::string& value) {
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

void AddElapsedMilliseconds(std::string* response, double elapsed_ms) {
  if (response == nullptr ||
      response->find("\"ok\":true") == std::string::npos) {
    return;
  }
  const std::size_t object_end = response->rfind('}');
  if (object_end == std::string::npos) {
    return;
  }
  std::ostringstream value;
  value << std::fixed << std::setprecision(3) << elapsed_ms;
  response->insert(object_end, ",\"ms\":" + value.str());
}

int RunBatchCommand(int argc, char** argv) {
  std::optional<std::string> requested_echarts_path;
  bool realtime_clock = false;
  bool animation = false;
  canvas_engine::PngCompression png_compression =
      canvas_engine::PngCompression::kDefault;
  std::optional<double> epoch_ms;
  double settle_ms = 3000.0;
  std::string error_message;
  for (int index = 2; index < argc; ++index) {
    const std::string argument(argv[index]);
    if (argument == "--help") {
      PrintUsage(std::cout);
      return 0;
    }
    if (argument == "--realtime-clock") {
      realtime_clock = true;
      continue;
    }
    if (argument == "--animate") {
      animation = true;
      continue;
    }
    if (argument == "--epoch-ms") {
      double parsed = 0.0;
      if (index + 1 >= argc ||
          !ParseFiniteDouble(argv[++index], &parsed)) {
        std::cerr << "--epoch-ms requires a finite number" << std::endl;
        return 2;
      }
      epoch_ms = parsed;
      continue;
    }
    if (argument == "--settle-ms") {
      if (index + 1 >= argc ||
          !ParseFiniteDouble(argv[++index], &settle_ms) || settle_ms < 0.0) {
        std::cerr << "--settle-ms requires a non-negative number" << std::endl;
        return 2;
      }
      continue;
    }
    if (argument == "--png-compression") {
      if (index + 1 >= argc ||
          !ParsePngCompression(argv[++index], &png_compression)) {
        std::cerr << "--png-compression must be fast, default, or small"
                  << std::endl;
        return 2;
      }
      continue;
    }
    if (argument == "--echarts" && index + 1 < argc) {
      requested_echarts_path = argv[++index];
      continue;
    }
    std::cerr << "invalid batch option: " << argument << std::endl;
    PrintUsage(std::cerr);
    return 2;
  }
  if (!epoch_ms.has_value() &&
      !ReadEnvironmentEpoch(&epoch_ms, &error_message)) {
    std::cerr << error_message << std::endl;
    return 2;
  }

  std::filesystem::path echarts_path;
  if (!ResolveEChartsPath(requested_echarts_path, argv[0], &echarts_path,
                          &error_message)) {
    std::cerr << error_message << std::endl;
    return 1;
  }

  const float default_dpr = DefaultDevicePixelRatio();
  canvas_engine::ScriptEngine engine(
      MakeEngineOptions(false, realtime_clock, epoch_ms));
  canvas_engine::EChartsRenderer renderer(engine, echarts_path);
  if (!renderer.Initialize()) {
    std::cerr << renderer.last_error() << std::endl;
    return 1;
  }

  std::string request;
  while (std::getline(std::cin, request)) {
    const auto start = std::chrono::steady_clock::now();
    std::string response;
    if (!renderer.RenderBatchRequest(request, default_dpr, settle_ms, &response,
                                     animation, png_compression)) {
      response = "{\"ok\":false,\"error\":" +
                 QuoteJsonString(renderer.last_error()) + "}";
    }
    const auto elapsed = std::chrono::duration<double, std::milli>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    AddElapsedMilliseconds(&response, elapsed);
    std::cout << response << std::endl;
  }
  if (std::cin.bad()) {
    std::cerr << "failed while reading batch requests" << std::endl;
    return 1;
  }
  return 0;
}

}  // namespace

int RunCli(int argc, char** argv) {
  if (argc <= 1) {
    PrintUsage(std::cerr);
    return 2;
  }

  const std::string command(argv[1]);
  if (command == "--help") {
    PrintUsage(std::cout);
    return 0;
  }
  if (command == "run") {
    return RunCommand(argc, argv, 2);
  }
  if (command == "render") {
    return RunRenderCommand(argc, argv);
  }
  if (command == "batch") {
    return RunBatchCommand(argc, argv);
  }
  return RunCommand(argc, argv, 1);
}
