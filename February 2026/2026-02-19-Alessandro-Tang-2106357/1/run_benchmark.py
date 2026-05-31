#!/usr/bin/env python3
import subprocess
import statistics
import sys
from collections import defaultdict

RUNS = int(sys.argv[1]) if len(sys.argv) > 1 else 20

results = defaultdict(list)

for i in range(RUNS):
    subprocess.run(["./main"], capture_output=True)
    with open("results.txt") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) == 2:
                key, val = parts
                try:
                    results[key].append(float(val))
                except ValueError:
                    pass
    print(f"Run {i+1}/{RUNS}", end="\r")

print()
print("Medians:")
for key, vals in results.items():
    print(f"  {key} {statistics.median(vals)}")