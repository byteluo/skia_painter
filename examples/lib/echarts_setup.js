loadScript("../../node_modules/echarts/dist/echarts.js");

(() => {
  function createCanvas(width, height) {
    const canvas = new Canvas(width, height);
    canvas.style = {};
    return canvas;
  }

  function renderChart({ option, width, height, output, afterSetOption }) {
    echarts.setPlatformAPI({
      createCanvas() {
        return createCanvas(width, height);
      },
      loadImage(src, onload) {
        const image = new Image();
        if (typeof onload === "function") {
          image.onload = onload;
        }
        image.src = src;
        return image;
      }
    });

    const canvas = createCanvas(width, height);
  const chart = echarts.init(canvas, null, {
    renderer: "canvas",
    width,
    height
  });
  chart.setOption(option);
  if (typeof afterSetOption === "function") {
    afterSetOption(chart);
  }
  chart.getZr().refreshImmediately();
  canvas.saveToPng(output);
  chart.dispose();
}

  return { renderChart, createCanvas };
})();
