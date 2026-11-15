loadScript("../../node_modules/echarts/dist/echarts.js");

(() => {
  function createCanvas(width, height) {
    const canvas = new Canvas(width, height);
    canvas.style = {};
    return canvas;
  }

  function renderChart({
    option,
    width,
    height,
    output,
    afterSetOption,
    captureTimeMs = 0
  }) {
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

    const zr = chart.getZr();
    const hasCaptureTime = Number.isFinite(captureTimeMs) && captureTimeMs > 0;
    if (hasCaptureTime) {
      const animation = zr.animation;
      animation.update(true);
      const getTime = Date.prototype.getTime;
      Date.prototype.getTime = function() {
        return getTime.call(this) + captureTimeMs;
      };
      try {
        animation.update(true);
        animation.update(true);
        zr.refreshImmediately();
      } finally {
        Date.prototype.getTime = getTime;
      }
    } else {
      zr.refreshImmediately();
    }

    try {
      canvas.saveToPng(output, !hasCaptureTime);
    } finally {
      chart.dispose();
    }
  }

  return { renderChart, createCanvas };
})();
