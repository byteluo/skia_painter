const echarts = require('echarts');

const ctxCalls = new Map();
const canvasCalls = new Map();

function count(map, key) {
  map.set(key, (map.get(key) || 0) + 1);
}

function makeGradient() {
  return {
    addColorStop(offset, color) {
      count(ctxCalls, `gradient.addColorStop(${typeof offset},${typeof color})`);
    }
  };
}

function makeContext() {
  const state = {
    font: '12px sans-serif',
    textAlign: 'left',
    textBaseline: 'alphabetic',
    shadowBlur: 0,
    shadowColor: '#000',
    shadowOffsetX: 0,
    shadowOffsetY: 0,
    lineWidth: 1,
    strokeStyle: '#000',
    fillStyle: '#000',
    globalAlpha: 1
  };

  return new Proxy(state, {
    get(target, prop) {
      if (typeof prop === 'symbol') {
        return target[prop];
      }
      if (prop in target) {
        return target[prop];
      }
      if (prop === 'measureText') {
        return (text) => {
          count(ctxCalls, 'measureText');
          const width = String(text ?? '').length * 8;
          return {
            width,
            actualBoundingBoxAscent: 8,
            actualBoundingBoxDescent: 2
          };
        };
      }
      if (prop === 'createLinearGradient' || prop === 'createRadialGradient') {
        return (...args) => {
          count(ctxCalls, `${prop}/${args.length}`);
          return makeGradient();
        };
      }
      if (prop === 'createPattern') {
        return (...args) => {
          count(ctxCalls, `${prop}/${args.length}`);
          return {};
        };
      }
      return (...args) => {
        count(ctxCalls, `${String(prop)}/${args.length}`);
        return undefined;
      };
    },
    set(target, prop, value) {
      target[prop] = value;
      count(ctxCalls, `set:${String(prop)}`);
      return true;
    }
  });
}

function makeCanvas(width, height) {
  const ctx = makeContext();
  ctx.canvas = null;
  return {
    width,
    height,
    style: {},
    nodeName: 'CANVAS',
    getContext(type) {
      count(canvasCalls, `getContext:${type}`);
      return ctx;
    },
    setAttribute(name, value) {
      count(canvasCalls, `setAttribute:${name}`);
      this[name] = value;
    },
    addEventListener() {},
    removeEventListener() {}
  };
}

const width = 640;
const height = 480;

echarts.setPlatformAPI({
  createCanvas() {
    count(canvasCalls, 'platform.createCanvas');
    return makeCanvas(width, height);
  },
  loadImage(src, onload) {
    count(canvasCalls, 'platform.loadImage');
    const image = { width: 0, height: 0, src };
    if (typeof onload === 'function') {
      onload();
    }
    return image;
  }
});

const canvas = makeCanvas(width, height);
const chart = echarts.init(canvas, null, {
  renderer: 'canvas',
  width,
  height
});

chart.setOption({
  animation: false,
  title: {
    text: 'Canvas API Probe'
  },
  tooltip: {},
  xAxis: {
    type: 'category',
    data: ['Mon', 'Tue', 'Wed', 'Thu', 'Fri']
  },
  yAxis: {
    type: 'value'
  },
  series: [
    {
      type: 'bar',
      data: [23, 45, 31, 12, 18]
    }
  ]
});

chart.getZr().refreshImmediately();

console.log('Canvas calls:');
for (const [name, times] of [...canvasCalls.entries()].sort()) {
  console.log(`  ${name}: ${times}`);
}

console.log('Context calls:');
for (const [name, times] of [...ctxCalls.entries()].sort()) {
  console.log(`  ${name}: ${times}`);
}
