function countInk(context) {
  const image = context.getImageData(0, 0, 160, 60);
  let count = 0;
  for (let i = 0; i < image.data.length; i += 4) {
    if (image.data[i] < 128 && image.data[i + 1] < 128 &&
        image.data[i + 2] < 128) {
      count += 1;
    }
  }
  return count;
}

function render(font) {
  const canvas = new Canvas(160, 60);
  const context = canvas.getContext("2d");
  context.fillStyle = "#ffffff";
  context.fillRect(0, 0, 160, 60);
  context.fillStyle = "#000000";
  context.font = font;
  context.textBaseline = "middle";
  context.fillText("Bold", 10, 30);
  return countInk(context);
}

const regularInk = render("40px sans-serif");
const boldInk = render("bold 40px sans-serif");
if (boldInk <= regularInk * 1.1) {
  throw new Error(`bold ink ${boldInk} was not >10% above regular ${regularInk}`);
}
print("font_weight_regression ok");
