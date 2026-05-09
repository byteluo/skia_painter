#include <iostream>
#include <string>

#include "canvas_engine/runtime/ScriptEngine.h"
#include "v8.h"

int main(int argc, char** argv) {
  int script_arg = 1;
  if (argc > 1 && std::string(argv[1]) == "--expose-gc") {
    v8::V8::SetFlagsFromString("--expose-gc");
    script_arg = 2;
  }

  if (argc <= script_arg) {
    std::cerr << "usage: canvas_engine [--expose-gc] <script.js>" << std::endl;
    return 1;
  }

  canvas_engine::ScriptEngine engine;
  if (!engine.RunScriptFile(argv[script_arg])) {
    return 1;
  }

  return 0;
}
