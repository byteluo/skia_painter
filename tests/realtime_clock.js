const start = performance.now();
const startDate = Date.now();
while (performance.now() === start) {}

if (performance.now() <= start) {
  throw new Error("realtime performance.now did not advance");
}
if (Date.now() <= startDate) {
  throw new Error("Date.now did not follow the realtime clock");
}

print("realtime_clock ok");
