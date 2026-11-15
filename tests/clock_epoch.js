if (Date.now() !== 1700000000000) {
  throw new Error(`unexpected epoch: ${Date.now()}`);
}

print("clock_epoch ok");
