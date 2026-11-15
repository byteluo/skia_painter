#pragma once

#include <filesystem>
#include <string>

namespace canvas_engine {

class ScriptLoader {
 public:
  static std::filesystem::path ResolvePath(
      const std::string& script_path, const std::filesystem::path& base_path);
  static bool LoadScript(const std::filesystem::path& script_path,
                         std::string* out_source);
};

}  // namespace canvas_engine
