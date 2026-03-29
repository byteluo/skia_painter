#include <iostream>
#include <string>

#include "canvas_engine/runtime/ScriptEngine.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: canvas_engine <script.js>" << std::endl;
    return 1;
  }

  canvas_engine::ScriptEngine engine;
  if (!engine.RunScriptFile(argv[1])) {
    return 1;
  }

  return 0;
}
