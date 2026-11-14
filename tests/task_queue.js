function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

const start = performance.now();
const order = [];
const frameTimes = [];
let cancelledFired = false;

const cancelledTimer = setTimeout(() => {
  cancelledFired = true;
}, 5);
clearTimeout(cancelledTimer);

setTimeout(() => order.push("50ms"), 50);
setTimeout(() => order.push("10ms"), 10);
setTimeout(() => order.push("equal-1"), 20);
setTimeout(() => order.push("equal-2"), 20);
setTimeout(() => {
  order.push("nested-parent");
  setTimeout(() => order.push("nested-child"), 0);
}, 15);

function recordFrame(timestamp) {
  frameTimes.push(timestamp);
  if (frameTimes.length < 4) {
    requestAnimationFrame(recordFrame);
  }
}
requestAnimationFrame(recordFrame);

setTimeout((first, second) => {
  const elapsed = performance.now() - start;
  assert(elapsed >= 100, `performance.now() advanced only ${elapsed}ms`);
  assert(first === "first" && second === 42, "setTimeout extra args mismatch");
  assert(!cancelledFired, "clearTimeout did not cancel the timer");

  const indexOf = (value) => order.indexOf(value);
  assert(indexOf("10ms") < indexOf("nested-parent"), "10ms timer ran late");
  assert(indexOf("nested-parent") < indexOf("nested-child"),
         "nested timer did not run after its parent");
  assert(indexOf("nested-child") < indexOf("equal-1"),
         "nested zero-delay timer ran late");
  assert(indexOf("equal-1") < indexOf("equal-2"),
         "equal-delay timers did not preserve enqueue order");
  assert(indexOf("equal-2") < indexOf("50ms"), "50ms timer ran early");
  assert(frameTimes.length === 4, "expected four animation frames");
  for (let i = 1; i < frameTimes.length; ++i) {
    const delta = frameTimes[i] - frameTimes[i - 1];
    assert(delta > 0, "animation frame timestamps did not increase");
    assert(Math.abs(delta - 1000 / 60) < 0.1,
           `animation frame interval was ${delta}ms`);
  }
}, 100, "first", 42);

function endlessAnimationFrame() {
  requestAnimationFrame(endlessAnimationFrame);
}
requestAnimationFrame(endlessAnimationFrame);

print("task_queue ok");
