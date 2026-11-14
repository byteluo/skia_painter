function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

const tinyRedPng =
  "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR4nGP4z8DwHwAFAAH/iZk9HQAAAABJRU5ErkJggg==";
let synchronous = true;
let dataLoaded = false;
let dataFailed = false;
let badDataFailed = false;
let percentLoaded = false;
let fileLoaded = false;
let latestError = false;
let staleLoad = false;

const canvas = new Canvas(12, 12);
const context = canvas.getContext("2d");
const image = new Image();
image.onload = function() {
  assert(!synchronous, "data URL onload fired synchronously");
  assert(image.complete, "image was incomplete in onload");
  assert(image.width === 1 && image.height === 1,
         `unexpected data URL dimensions ${image.width}x${image.height}`);
  context.drawImage(image, 0, 0, 1, 1, 0, 0, 12, 12);
  const pixel = context.getImageData(6, 6, 1, 1).data;
  assert(pixel[0] > 230 && pixel[1] < 20 && pixel[2] < 20 && pixel[3] > 230,
         `data URL pixel was rgba(${pixel[0]},${pixel[1]},${pixel[2]},${pixel[3]})`);
  dataLoaded = true;
};
image.onerror = function() {
  dataFailed = true;
  throw new Error("valid data URL failed to decode");
};
image.src = tinyRedPng;
assert(image.complete && image.width === 1 && image.height === 1,
       "data URL dimensions or complete were not set before dispatch");

const badImage = new Image();
badImage.onerror = function() {
  assert(!synchronous, "data URL onerror fired synchronously");
  assert(badImage.complete, "failed image was incomplete in onerror");
  badDataFailed = true;
};
badImage.src = "data:image/png;base64,not-valid!";

const percentImage = new Image();
percentImage.onload = function() {
  assert(!synchronous, "percent-encoded data URL onload fired synchronously");
  assert(percentImage.width === 1 && percentImage.height === 1,
         "percent-encoded data URL has invalid dimensions");
  percentLoaded = true;
};
percentImage.onerror = function() {
  throw new Error("percent-encoded PNG failed to load");
};
percentImage.src =
  "data:image/png,%89PNG%0D%0A%1A%0A%00%00%00%0DIHDR%00%00%00%01%00%00%00%01%08%06%00%00%00%1F%15%C4%89%00%00%00%0DIDATx%9Cc%F8%CF%C0%F0%1F%00%05%00%01%FF%89%99%3D%1D%00%00%00%00IEND%AEB%60%82";

const fileImage = new Image();
fileImage.onload = function() {
  assert(!synchronous, "file onload fired synchronously");
  assert(fileImage.width > 0 && fileImage.height > 0,
         "file image has invalid dimensions");
  fileLoaded = true;
};
fileImage.onerror = function() {
  throw new Error("existing PNG file failed to load");
};
fileImage.src = "../output/echarts_bar.png";
assert(fileImage.complete && fileImage.width > 0 && fileImage.height > 0,
       "file image did not load before dispatch");

const reassignedImage = new Image();
reassignedImage.onload = function() {
  staleLoad = true;
};
reassignedImage.onerror = function() {
  assert(!synchronous, "reassigned image onerror fired synchronously");
  latestError = true;
};
reassignedImage.src = tinyRedPng;
reassignedImage.src = "data:image/png;base64,invalid";

synchronous = false;
setTimeout(function() {
  assert(dataLoaded, "data URL onload did not fire");
  assert(!dataFailed, "valid data URL dispatched onerror");
  assert(badDataFailed, "invalid data URL did not dispatch onerror");
  assert(percentLoaded, "percent-encoded data URL did not load");
  assert(fileLoaded, "file path onload did not fire");
  assert(latestError, "latest reassigned source did not fire onerror");
  assert(!staleLoad, "stale source dispatched after reassignment");
  print("image_loading ok");
}, 0);
