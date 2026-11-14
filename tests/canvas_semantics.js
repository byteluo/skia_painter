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

print("canvas_semantics ok");
