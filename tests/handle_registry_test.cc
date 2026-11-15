#include "HandleRegistry.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
  canvas_engine::HandleRegistry registry;
  int canvas = 0;
  int image = 0;
  const canvas_engine::HandleRegistry::Header canvas_header{1, 17};
  const canvas_engine::HandleRegistry::Header image_header{4, 18};

  assert(registry.Register(&canvas_header, &canvas));
  assert(registry.Register(&image_header, &image));
  assert(registry.Lookup(&canvas_header, 1) == &canvas);
  assert(registry.Lookup(&canvas_header, 4) == nullptr);
  assert(registry.Lookup(&image_header, 4) == &image);
  assert(!registry.Register(&canvas_header, &image));
  assert(!registry.Remove(17, 4));
  assert(registry.Remove(17, 1));
  assert(registry.Lookup(&canvas_header, 1) == nullptr);

  std::cout << "handle_registry ok\n";
  return 0;
}
