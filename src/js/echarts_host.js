let __canvasEngineRenderWidth = 960;
let __canvasEngineRenderHeight = 540;
let __canvasEngineRenderDpr = 3;

function __canvasEngineCreateCanvas(width, height, dpr) {
  const canvas = new Canvas(width, height, dpr);
  canvas.style = {};
  return canvas;
}

echarts.setPlatformAPI({
  createCanvas() {
    return __canvasEngineCreateCanvas(
      __canvasEngineRenderWidth,
      __canvasEngineRenderHeight,
      __canvasEngineRenderDpr
    );
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

globalThis.__renderChart = function(option, renderOptions, outPath) {
  const width = renderOptions.width;
  const height = renderOptions.height;
  const dpr = renderOptions.dpr;
  __canvasEngineRenderWidth = width;
  __canvasEngineRenderHeight = height;
  __canvasEngineRenderDpr = dpr;

  let renderOption = option;
  if (renderOptions.background && option &&
      typeof option === "object" && option.backgroundColor == null) {
    renderOption = Object.assign({}, option, {
      backgroundColor: renderOptions.background
    });
  }

  const canvas = __canvasEngineCreateCanvas(width, height, dpr);
  const chart = echarts.init(
    canvas,
    renderOptions.theme || null,
    { renderer: "canvas", width, height }
  );
  try {
    chart.setOption(renderOption);
    chart.getZr().refreshImmediately();
    canvas.saveToPng(outPath);
    return { out: outPath, width, height, dpr };
  } finally {
    chart.dispose();
  }
};
