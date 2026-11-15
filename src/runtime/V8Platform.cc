#include "canvas_engine/runtime/V8Platform.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include "libplatform/libplatform.h"
#include "v8.h"

namespace canvas_engine {

namespace {

struct PlatformState {
  std::unique_ptr<v8::Platform> platform;
  bool initialized = false;

  ~PlatformState() {
    if (initialized) {
      v8::V8::Dispose();
      v8::V8::DisposePlatform();
    }
  }
};

PlatformState& GetPlatformState() {
  static PlatformState state;
  return state;
}

void V8FatalError(const char* location, int line, const char* message) {
  std::cerr << "V8 fatal error";
  if (location != nullptr) {
    std::cerr << " at " << location;
    if (line > 0) {
      std::cerr << ":" << line;
    }
  }
  if (message != nullptr) {
    std::cerr << ": " << message;
  }
  std::cerr << std::endl;
}

std::string DetectV8DataDir() {
  if (const char* env_dir = std::getenv("V8_ICU_DATA_DIR")) {
    const std::filesystem::path env_path(env_dir);
    if (*env_dir != '\0' &&
        std::filesystem::exists(env_path / "icudtl.dat")) {
      return env_dir;
    }

    std::cerr << "[canvas_engine] Warning: V8_ICU_DATA_DIR='"
              << (*env_dir == '\0' ? "<empty>" : env_dir)
              << "' does not contain icudtl.dat; falling back to built-in "
                 "V8 data directories."
              << std::endl;
  }

  constexpr const char* kCandidateDirs[] = {
#if defined(__APPLE__)
      "/opt/homebrew/opt/v8/libexec",
      "/usr/local/opt/v8/libexec",
#endif
      "/usr/share/v8",
      "/usr/lib/v8",
      "/usr/local/lib/v8",
      "/usr/local/share/v8",
  };
  for (const char* dir : kCandidateDirs) {
    if (std::filesystem::exists(std::filesystem::path(dir) / "icudtl.dat")) {
      return dir;
    }
  }
  return {};
}

std::once_flag& InitializationFlag() {
  static std::once_flag flag;
  return flag;
}

}  // namespace

void V8Platform::Initialize() {
  PlatformState& state = GetPlatformState();
  std::call_once(InitializationFlag(), [&state]() {
    v8::V8::SetFatalErrorHandler(V8FatalError);

    const std::string data_dir = DetectV8DataDir();
    if (!data_dir.empty()) {
      const auto icu_data_path =
          (std::filesystem::path(data_dir) / "icudtl.dat").string();
      v8::V8::InitializeICU(icu_data_path.c_str());
      const auto snapshot_path =
          (std::filesystem::path(data_dir) / "snapshot_blob.bin").string();
      if (std::filesystem::exists(snapshot_path)) {
        v8::V8::InitializeExternalStartupDataFromFile(snapshot_path.c_str());
      }
    } else {
      v8::V8::InitializeICUDefaultLocation(nullptr);
      v8::V8::InitializeExternalStartupData(nullptr);
    }

    state.platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(state.platform.get());
    if (!v8::V8::Initialize()) {
      throw std::runtime_error("failed to initialize V8");
    }
    state.initialized = true;
  });
}

}  // namespace canvas_engine
