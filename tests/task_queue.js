const start = performance.now();

setTimeout(() => {
  const elapsed = performance.now() - start;
  print(`timeout_elapsed=${Math.floor(elapsed)}`);
  if (elapsed < 100) {
    throw new Error(`setTimeout fired too early: ${elapsed}`);
  }
}, 100);

while (performance.now() - start < 120) {
}

requestAnimationFrame(function tick() {
  requestAnimationFrame(tick);
});
