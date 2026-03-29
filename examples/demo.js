const canvas = new Canvas(960, 540);
const ctx = canvas.getContext("2d");

ctx.fillStyle = "#f5efe6";
ctx.fillRect(0, 0, canvas.width, canvas.height);

ctx.fillStyle = "#1f2937";
ctx.fillRect(72, 72, 816, 396);

ctx.save();
ctx.translate(480, 270);
ctx.rotate(Math.PI / 14);
ctx.fillStyle = "#d97706";
ctx.fillRect(-180, -48, 360, 96);
ctx.restore();

ctx.strokeStyle = "#f9fafb";
ctx.lineWidth = 12;
ctx.beginPath();
ctx.moveTo(120, 420);
ctx.lineTo(320, 180);
ctx.lineTo(560, 360);
ctx.lineTo(820, 140);
ctx.stroke();

ctx.fillStyle = "rgba(59, 130, 246, 0.82)";
ctx.beginPath();
ctx.arc(240, 160, 56, 0, Math.PI * 2);
ctx.fill();

ctx.fillStyle = "#22c55e";
ctx.beginPath();
ctx.arc(720, 360, 92, 0, Math.PI * 2);
ctx.fill();

canvas.saveToPng("output/demo.png");
console.log("wrote output/demo.png");
