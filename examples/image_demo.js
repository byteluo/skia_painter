const stamp = new Canvas(96, 96);
const sctx = stamp.getContext("2d");
sctx.fillStyle = "#0f172a";
sctx.fillRect(0, 0, stamp.width, stamp.height);
sctx.fillStyle = "#f97316";
sctx.beginPath();
sctx.arc(48, 48, 26, 0, Math.PI * 2);
sctx.fill();
sctx.strokeStyle = "#f8fafc";
sctx.lineWidth = 4;
sctx.stroke();
stamp.saveToPng("../output/image_demo_source.png");

const image = new Image();
image.src = "../output/image_demo_source.png";

const canvas = new Canvas(420, 180);
const ctx = canvas.getContext("2d");
ctx.fillStyle = "#e2e8f0";
ctx.fillRect(0, 0, canvas.width, canvas.height);

ctx.drawImage(image, 20, 20);
ctx.drawImage(image, 140, 20, 120, 120);
ctx.drawImage(image, 12, 12, 72, 72, 300, 34, 88, 88);

canvas.saveToPng("../output/image_demo.png");
console.log("wrote ../output/image_demo_source.png");
console.log("wrote ../output/image_demo.png");
