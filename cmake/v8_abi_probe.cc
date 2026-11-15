#include <cstdlib>
#include <filesystem>
#include <memory>

#include "libplatform/libplatform.h"
#include "v8.h"

int main() {
  const char* data_dir = std::getenv("V8_ICU_DATA_DIR");
  if (data_dir) {
    const std::filesystem::path directory(data_dir);
    const auto icu_path = directory / "icudtl.dat";
    v8::V8::InitializeICU(icu_path.c_str());
    const auto snapshot_path = directory / "snapshot_blob.bin";
    if (std::filesystem::exists(snapshot_path)) {
      v8::V8::InitializeExternalStartupDataFromFile(snapshot_path.c_str());
    }
  } else {
    v8::V8::InitializeICUDefaultLocation(nullptr);
    v8::V8::InitializeExternalStartupData(nullptr);
  }

  auto platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  if (!v8::V8::Initialize()) {
    return 1;
  }

  auto allocator = std::unique_ptr<v8::ArrayBuffer::Allocator>(
      v8::ArrayBuffer::Allocator::NewDefaultAllocator());
  v8::Isolate::CreateParams params;
  params.array_buffer_allocator = allocator.get();
  v8::Isolate* isolate = v8::Isolate::New(params);
  if (!isolate) {
    return 2;
  }

  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    if (v8::Context::New(isolate).IsEmpty()) {
      isolate->Dispose();
      return 3;
    }
  }

  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::DisposePlatform();
  return 0;
}
