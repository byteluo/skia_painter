const canvas = new Canvas(4, 4);
const context = canvas.getContext("2d");

function drawForever() {
  context.fillRect(0, 0, 1, 1);
  requestAnimationFrame(drawForever);
}

requestAnimationFrame(drawForever);
print("drawing animation-frame loop scheduled");
