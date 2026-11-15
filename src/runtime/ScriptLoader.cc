#include "ScriptLoader.h"

#include <fstream>
#include <sstream>

namespace canvas_engine {

std::filesystem::path ScriptLoader::ResolvePath(
    const std::string& script_path, const std::filesystem::path& base_path) {
  std::filesystem::path path(script_path);
  if (path.is_absolute()) {
    return path.lexically_normal();
  }
  return (base_path / path).lexically_normal();
}

bool ScriptLoader::LoadScript(const std::filesystem::path& script_path,
                              std::string* out_source) {
  std::ifstream input(script_path, std::ios::in | std::ios::binary);
  if (!input) {
    return false;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  *out_source = buffer.str();
  return true;
}

}  // namespace canvas_engine
