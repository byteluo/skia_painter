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
  let finished = false;
  let settleTimer = 0;
  const finish = () => {
    if (finished) return;
    finished = true;
    chart.off("finished", finish);
    if (settleTimer) clearTimeout(settleTimer);
    try {
      canvas.saveToPng(outPath);
    } finally {
      chart.dispose();
    }
  };
  chart.on("finished", finish);
  try {
    chart.setOption(renderOption);
    chart.getZr().refreshImmediately();
    if (!finished) {
      settleTimer = setTimeout(finish, renderOptions.settleMs);
    }
  } catch (error) {
    chart.off("finished", finish);
    chart.dispose();
    throw error;
  }
  return { out: outPath, width, height, dpr };
};
