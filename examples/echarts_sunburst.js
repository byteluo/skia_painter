loadScript("../node_modules/echarts/dist/echarts.js");

const width = 960;
const height = 540;

function createCanvas(targetWidth, targetHeight) {
  const canvas = new Canvas(targetWidth, targetHeight);
  canvas.style = {};
  return canvas;
}

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

const data = [
  {
    name: "Traffic",
    value: 68,
    itemStyle: { color: "#2563eb" },
    children: [
      {
        name: "Search",
        value: 42,
        itemStyle: { color: "#3b82f6" },
        children: [
          { name: "Organic", value: 24, itemStyle: { color: "#60a5fa" } },
          { name: "Ads", value: 18, itemStyle: { color: "#93c5fd" } }
        ]
      },
      {
        name: "Social",
        value: 26,
        itemStyle: { color: "#1d4ed8" },
        children: [
          { name: "Video", value: 12, itemStyle: { color: "#38bdf8" } },
          { name: "Community", value: 14, itemStyle: { color: "#0ea5e9" } }
        ]
      }
    ]
  },
  {
    name: "Sales",
    value: 56,
    itemStyle: { color: "#f97316" },
    children: [
      {
        name: "Direct",
        value: 34,
        itemStyle: { color: "#fb923c" },
        children: [
          { name: "Store", value: 15, itemStyle: { color: "#fdba74" } },
          { name: "Partners", value: 19, itemStyle: { color: "#fed7aa" } }
        ]
      },
      {
        name: "Campaigns",
        value: 22,
        itemStyle: { color: "#ea580c" },
        children: [
          { name: "Launch", value: 10, itemStyle: { color: "#fb7185" } },
          { name: "Seasonal", value: 12, itemStyle: { color: "#f43f5e" } }
        ]
      }
    ]
  },
  {
    name: "Retention",
    value: 46,
    itemStyle: { color: "#14b8a6" },
    children: [
      {
        name: "Product",
        value: 28,
        itemStyle: { color: "#2dd4bf" },
        children: [
          { name: "Quality", value: 16, itemStyle: { color: "#5eead4" } },
          { name: "Pricing", value: 12, itemStyle: { color: "#99f6e4" } }
        ]
      },
      {
        name: "Support",
        value: 18,
        itemStyle: { color: "#0f766e" },
        children: [
          { name: "Docs", value: 8, itemStyle: { color: "#34d399" } },
          { name: "Success", value: 10, itemStyle: { color: "#10b981" } }
        ]
      }
    ]
  }
];

chart.setOption({
  animation: false,
  backgroundColor: "#fffaf0",
  title: {
    text: "Sunburst Compatibility",
    subtext: "Sector layout + rotated labels",
    left: "center",
    top: 18,
    textStyle: {
      color: "#1f2937",
      fontSize: 24,
      fontWeight: "bold"
    },
    subtextStyle: {
      color: "#6b7280"
    }
  },
  series: [
    {
      type: "sunburst",
      radius: ["16%", "78%"],
      center: ["50%", "57%"],
      sort: null,
      nodeClick: false,
      itemStyle: {
        borderColor: "#fffaf0",
        borderWidth: 3
      },
      label: {
        rotate: "radial",
        color: "#111827",
        fontSize: 12
      },
      levels: [
        {},
        {
          r0: "16%",
          r: "36%",
          label: {
            rotate: 0,
            fontWeight: "bold"
          }
        },
        {
          r0: "36%",
          r: "58%"
        },
        {
          r0: "58%",
          r: "78%",
          label: {
            fontSize: 10
          }
        }
      ],
      data
    }
  ]
});

chart.getZr().refreshImmediately();
canvas.saveToPng("../output/echarts_sunburst.png");
console.log("wrote ../output/echarts_sunburst.png");
