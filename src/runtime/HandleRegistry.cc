#include "HandleRegistry.h"

namespace canvas_engine {

bool HandleRegistry::Register(const Header* header, void* handle) {
  if (header == nullptr || handle == nullptr) {
    return false;
  }
  return handles_.emplace(header->id, Entry{header->tag, handle}).second;
}

void* HandleRegistry::Lookup(const Header* header,
                             std::uint32_t expected_tag) const {
  if (header == nullptr || header->tag != expected_tag) {
    return nullptr;
  }
  const auto it = handles_.find(header->id);
  if (it == handles_.end() || it->second.tag != expected_tag) {
    return nullptr;
  }
  return it->second.handle;
}

bool HandleRegistry::Remove(std::uint64_t id, std::uint32_t expected_tag) {
  const auto it = handles_.find(id);
  if (it == handles_.end() || it->second.tag != expected_tag) {
    return false;
  }
  handles_.erase(it);
  return true;
}

}  // namespace canvas_engine
