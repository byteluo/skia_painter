#include <iostream>

#include "canvas_engine/runtime/ScriptEngine.h"

int main() {
  canvas_engine::ScriptEngine first;
  canvas_engine::ScriptEngine second;

  if (!first.RunScriptSource("globalThis.engineName = 'first';", "first.js")) {
    std::cerr << first.last_error() << std::endl;
    return 1;
  }
  if (!second.RunScriptSource("globalThis.engineName = 'second';", "second.js")) {
    std::cerr << second.last_error() << std::endl;
    return 1;
  }
  if (!first.RunScriptSource(
          "if (engineName !== 'first') throw new Error('first engine state leaked');",
          "first-check.js")) {
    std::cerr << first.last_error() << std::endl;
    return 1;
  }
  if (!second.RunScriptSource(
          "if (engineName !== 'second') throw new Error('second engine state leaked');",
          "second-check.js")) {
    std::cerr << second.last_error() << std::endl;
    return 1;
  }

  std::cout << "multi_engine ok" << std::endl;
  return 0;
}
