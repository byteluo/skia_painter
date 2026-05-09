const canvas = new Canvas(320, 220);
const ctx = canvas.getContext("2d");

ctx.fillStyle = "white";
ctx.fillRect(0, 0, canvas.width, canvas.height);

ctx.fillStyle = "black";
ctx.font = "32px sans-serif";
ctx.translate(80, 80);
ctx.rotate(Math.PI / 6);
ctx.fillText("hello", 0, 0);

canvas.saveToPng("../output/text_rotate.png");
print("wrote output/text_rotate.png");
