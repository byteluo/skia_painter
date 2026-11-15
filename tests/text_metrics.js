function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function inkBounds(baseline) {
  const canvas = new Canvas(100, 80);
  const context = canvas.getContext("2d");
  context.fillStyle = "#ffffff";
  context.fillRect(0, 0, canvas.width, canvas.height);
  context.fillStyle = "#000000";
  context.font = '20px "Noto Sans"';
  context.textBaseline = baseline;
  context.fillText("H", 20, 40);

  const image = context.getImageData(0, 0, canvas.width, canvas.height);
  let top = canvas.height;
  for (let y = 0; y < canvas.height; ++y) {
    for (let x = 0; x < canvas.width; ++x) {
      const offset = (y * canvas.width + x) * 4;
      if (image.data[offset] < 128 && image.data[offset + 3] > 0) {
        top = Math.min(top, y);
      }
    }
  }
  return top;
}

const canvas = new Canvas(100, 80);
const context = canvas.getContext("2d");
context.font = '20px "Noto Sans"';
const metrics = context.measureText("Hello");
assert(metrics.width > 40 && metrics.width < 60,
       `unexpected Noto Sans width: ${metrics.width}`);
assert(metrics.fontBoundingBoxAscent > 0,
       "fontBoundingBoxAscent must be positive");
assert(metrics.actualBoundingBoxAscent > 0,
       "actualBoundingBoxAscent must be positive");
assert(metrics.actualBoundingBoxRight > 0,
       "actualBoundingBoxRight must be positive");
assert(metrics.alphabeticBaseline === 0,
       "alphabetic baseline should be the default reference");
context.textBaseline = "top";
const topMetrics = context.measureText("Hello");
assert(topMetrics.alphabeticBaseline < 0,
       "alphabetic baseline should be below the top reference");
assert(topMetrics.fontBoundingBoxAscent === 0 &&
       topMetrics.emHeightAscent === 0,
       "top baseline should align with the top of the font metrics");
assert(topMetrics.actualBoundingBoxDescent >
       metrics.actualBoundingBoxDescent,
       "top baseline should move the glyph bounds below the reference");
context.textBaseline = "alphabetic";

const topInk = inkBounds("top");
const hangingInk = inkBounds("hanging");
const middleInk = inkBounds("middle");
const alphabeticInk = inkBounds("alphabetic");
const ideographicInk = inkBounds("ideographic");
const bottomInk = inkBounds("bottom");
assert(topInk > hangingInk && hangingInk > middleInk &&
       middleInk > alphabeticInk && alphabeticInk > ideographicInk &&
       ideographicInk > bottomInk,
       "text baseline drawing shifts are inconsistent");
assert(topInk - bottomInk > 10,
       "top and bottom baselines did not shift the text");

print("text_metrics ok");
