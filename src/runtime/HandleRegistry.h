#pragma once

#include <cstdint>
#include <unordered_map>

namespace canvas_engine {

class HandleRegistry {
 public:
  struct Header {
    std::uint32_t tag = 0;
    std::uint64_t id = 0;
  };

  bool Register(const Header* header, void* handle);
  void* Lookup(const Header* header, std::uint32_t expected_tag) const;
  bool Remove(std::uint64_t id, std::uint32_t expected_tag);

 private:
  struct Entry {
    std::uint32_t tag = 0;
    void* handle = nullptr;
  };

  std::unordered_map<std::uint64_t, Entry> handles_;
};

}  // namespace canvas_engine
