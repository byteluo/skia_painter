function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function pixel(context, x, y) {
  return context.getImageData(x, y, 1, 1).data;
}

const gradientCanvas = new Canvas(20, 20);
const gradientContext = gradientCanvas.getContext("2d");
const gradient = gradientContext.createLinearGradient(0, 0, 20, 0);
gradientContext.fillStyle = gradient;
gradient.addColorStop(0, "#00ff00");
gradient.addColorStop(1, "#00ff00");
gradientContext.fillRect(0, 0, 20, 20);
const gradientPixel = pixel(gradientContext, 10, 10);
assert(gradientPixel[0] < 20 && gradientPixel[1] > 230 &&
       gradientPixel[2] < 20 && gradientPixel[3] > 230,
       "gradient stops added after assignment were not applied");

function addNestedRects(context) {
  context.beginPath();
  context.rect(1, 1, 18, 18);
  context.rect(6, 6, 8, 8);
}

const nonzeroCanvas = new Canvas(20, 20);
const nonzeroContext = nonzeroCanvas.getContext("2d");
addNestedRects(nonzeroContext);
nonzeroContext.fill("nonzero");
assert(pixel(nonzeroContext, 10, 10)[3] > 230,
       "nonzero fill unexpectedly left a hole");

const evenoddCanvas = new Canvas(20, 20);
const evenoddContext = evenoddCanvas.getContext("2d");
addNestedRects(evenoddContext);
evenoddContext.fill("evenodd");
assert(pixel(evenoddContext, 10, 10)[3] === 0,
       "evenodd fill did not leave a hole");

const clipCanvas = new Canvas(20, 20);
const clipContext = clipCanvas.getContext("2d");
addNestedRects(clipContext);
clipContext.clip("evenodd");
clipContext.fillStyle = "#ff0000";
clipContext.fillRect(0, 0, 20, 20);
assert(pixel(clipContext, 3, 3)[3] > 230,
       "evenodd clip removed the outer region");
assert(pixel(clipContext, 10, 10)[3] === 0,
       "evenodd clip did not leave a hole");

for (const method of [
  (context) => context.fill("invalid"),
  (context) => context.clip("invalid")
]) {
  let threwTypeError = false;
  try {
    method(gradientContext);
  } catch (error) {
    threwTypeError = error instanceof TypeError;
  }
  assert(threwTypeError, "invalid fillRule did not throw TypeError");
}

const assignmentCanvas = new Canvas(20, 20);
const assignmentContext = assignmentCanvas.getContext("2d");
assignmentContext.fillStyle = "not-a-color";
assignmentContext.strokeStyle = {};
assignmentContext.lineWidth = Infinity;
assignmentContext.globalAlpha = NaN;
assignmentContext.textAlign = "middle";
assignmentContext.textBaseline = "invalid";
assignmentContext.globalCompositeOperation = "invalid";
assignmentContext.lineCap = "invalid";
assignmentContext.lineJoin = "invalid";
assignmentContext.miterLimit = -1;
assignmentContext.shadowBlur = Infinity;
assignmentContext.shadowColor = "not-a-color";
assignmentContext.shadowOffsetX = NaN;
assignmentContext.shadowOffsetY = Infinity;
assignmentContext.lineDashOffset = NaN;
assert(assignmentContext.fillStyle === "#000000" &&
       assignmentContext.strokeStyle === "#000000",
       "invalid color assignments were not ignored");
assert(assignmentContext.lineWidth === 1 &&
       assignmentContext.globalAlpha === 1,
       "invalid numeric assignments were not ignored");
assert(assignmentContext.textAlign === "start" &&
       assignmentContext.textBaseline === "alphabetic" &&
       assignmentContext.globalCompositeOperation === "source-over" &&
       assignmentContext.lineCap === "butt" &&
       assignmentContext.lineJoin === "miter",
       "invalid enum assignments were not ignored");
assert(assignmentContext.miterLimit === 10 &&
       assignmentContext.shadowBlur === 0 &&
       assignmentContext.shadowColor === "rgba(0, 0, 0, 0)" &&
       assignmentContext.shadowOffsetX === 0 &&
       assignmentContext.shadowOffsetY === 0 &&
       assignmentContext.lineDashOffset === 0,
       "invalid shadow or dash assignments were not ignored");

print("canvas_semantics ok");
