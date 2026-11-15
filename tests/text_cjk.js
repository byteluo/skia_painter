function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function hasInk(image, width, left, right) {
  for (let y = 0; y < image.height; ++y) {
    for (let x = left; x < right; ++x) {
      const offset = (y * width + x) * 4;
      if (image.data[offset] < 128 && image.data[offset + 3] > 0) {
        return true;
      }
    }
  }
  return false;
}

const canvas = new Canvas(320, 80);
const context = canvas.getContext("2d");
context.fillStyle = "#ffffff";
context.fillRect(0, 0, canvas.width, canvas.height);
context.fillStyle = "#000000";
context.font = '20px "Noto Sans"';
const text = "Sales 销售额 2026";
const latinWidth = context.measureText("Sales ").width;
const cjkWidth = context.measureText("销售额").width;
assert(cjkWidth > 40 && cjkWidth < 90,
       `unexpected CJK fallback width: ${cjkWidth}`);
context.fillText(text, 10, 40);

const image = context.getImageData(0, 0, canvas.width, canvas.height);
const latinLeft = 10;
const cjkLeft = Math.ceil(10 + latinWidth);
const cjkRight = Math.ceil(cjkLeft + cjkWidth);
assert(hasInk(image, canvas.width, latinLeft, cjkLeft),
       "Latin text produced no ink");
assert(hasInk(image, canvas.width, cjkLeft, cjkRight),
       "CJK text produced no ink");

print("text_cjk ok");
