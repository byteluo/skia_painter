for (let i = 0; i < 10000; i += 1) {
  new Canvas(100, 100).getContext("2d");
}

print(typeof gc === "function");

if (typeof gc !== "function") {
  throw new Error("gc must be exposed for handle GC regression");
}

for (let i = 0; i < 8; i += 1) {
  gc();
}

const counts = __canvasEngineHandleCounts();
print(`canvases=${counts.canvases}`);

if (counts.canvases >= 100) {
  throw new Error(`expected fewer than 100 live canvas handles, got ${counts.canvases}`);
}
