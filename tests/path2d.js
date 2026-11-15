function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function pixel(context, x, y) {
  return context.getImageData(x, y, 1, 1).data;
}

const emptyPath = new Path2D();
emptyPath.moveTo(1, 1);
emptyPath.lineTo(19, 1);
emptyPath.lineTo(19, 19);
emptyPath.lineTo(1, 19);
emptyPath.closePath();

const clonedPath = new Path2D(emptyPath);
const svgPath = new Path2D("M 1 1 L 19 1 L 19 19 L 1 19 Z");
const appendedPath = new Path2D();
appendedPath.addPath(svgPath);

const hitTestCanvas = new Canvas(20, 20);
const hitTestContext = hitTestCanvas.getContext("2d");
assert(hitTestContext.isPointInPath(emptyPath, 10, 10),
       "Path2D hit test missed an interior point");
assert(hitTestContext.isPointInPath(clonedPath, 10, 10),
       "Path2D copy constructor lost path data");
assert(hitTestContext.isPointInPath(svgPath, 10, 10),
       "SVG path constructor did not parse the path");
assert(hitTestContext.isPointInPath(appendedPath, 10, 10),
       "addPath did not append the source path");
assert(!hitTestContext.isPointInPath(svgPath, 25, 25),
       "Path2D hit test accepted an exterior point");

const rings = new Path2D();
rings.rect(1, 1, 18, 18);
rings.rect(6, 6, 8, 8);
assert(hitTestContext.isPointInPath(rings, 10, 10, "nonzero"),
       "nonzero Path2D hit test unexpectedly found a hole");
assert(!hitTestContext.isPointInPath(rings, 10, 10, "evenodd"),
       "evenodd Path2D hit test did not find the inner hole");

hitTestContext.beginPath();
hitTestContext.rect(2, 2, 4, 4);
assert(hitTestContext.isPointInPath(3, 3),
       "current-path isPointInPath overload failed");
assert(!hitTestContext.isPointInPath(10, 10),
       "current-path isPointInPath accepted an exterior point");

const fillCanvas = new Canvas(20, 20);
const fillContext = fillCanvas.getContext("2d");
fillContext.fillStyle = "#ff0000";
fillContext.fill(rings, "evenodd");
assert(pixel(fillContext, 3, 3)[3] > 230,
       "fill(path, rule) did not paint the outer ring");
assert(pixel(fillContext, 10, 10)[3] === 0,
       "fill(path, evenodd) did not leave a hole");

const strokeCanvas = new Canvas(20, 20);
const strokeContext = strokeCanvas.getContext("2d");
strokeContext.strokeStyle = "#00ff00";
strokeContext.lineWidth = 1;
strokeContext.stroke(svgPath);
assert(pixel(strokeContext, 1, 10)[3] > 0,
       "stroke(path) did not paint the path outline");
assert(pixel(strokeContext, 10, 10)[3] === 0,
       "stroke(path) filled the path interior");

const clipCanvas = new Canvas(20, 20);
const clipContext = clipCanvas.getContext("2d");
clipContext.clip(rings, "evenodd");
clipContext.fillStyle = "#0000ff";
clipContext.fillRect(0, 0, 20, 20);
assert(pixel(clipContext, 3, 3)[3] > 230,
       "clip(path, rule) removed the outer ring");
assert(pixel(clipContext, 10, 10)[3] === 0,
       "clip(path, evenodd) did not leave a hole");

print("path2d ok");
