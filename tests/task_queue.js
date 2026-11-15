function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

const start = performance.now();
const startDate = Date.now();
const order = [];
const frameTimes = [];
let cancelledFired = false;

for (let index = 0; index < 10000; ++index) {
  Math.sqrt(index);
}
assert(start === 0 && performance.now() === start,
       "performance.now changed during synchronous work");
assert(Date.now() === startDate,
       "Date.now changed during synchronous work");
assert(new Date() instanceof Date, "new Date() is not a Date instance");
assert(Date.prototype.constructor === Date,
       "Date.prototype.constructor is not Date");
assert(new Date(2020, 0, 1).getFullYear() === 2020,
       "Date constructor arguments were not preserved");
assert(typeof Date() === "string", "Date() did not return a string");
assert(Date.UTC(2020, 0, 1) === 1577836800000,
       "Date.UTC was not preserved");
assert(Date.parse("2020-01-01T00:00:00Z") === 1577836800000,
       "Date.parse was not preserved");

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
  assert(performance.now() === 100,
         `performance.now was ${performance.now()}ms when 100ms timer fired`);
  assert(Date.now() - startDate === 100,
         `Date.now advanced by ${Date.now() - startDate}ms for 100ms timer`);
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

let idleFrameCount = 0;
function endlessAnimationFrame(timestamp) {
  ++idleFrameCount;
  if (idleFrameCount === 60) {
    assert(timestamp >= 1000 && timestamp <= 1100,
           `idle animation-frame loop stopped at ${timestamp}ms`);
    print("idle animation-frame loop reached 60 frames");
  }
  requestAnimationFrame(endlessAnimationFrame);
}
setTimeout(() => requestAnimationFrame(endlessAnimationFrame), 100);

print("task_queue ok");
