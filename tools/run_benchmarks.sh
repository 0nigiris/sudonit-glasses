#!/usr/bin/env bash
# Run the AI evaluation across the four degradation conditions and collect results.
#
# Produces REAL accuracy/latency/token/cost numbers only with a real model:
#   ANTHROPIC_API_KEY=...  ./tools/run_benchmarks.sh
# Without a key it runs the offline stub (structure only — no model numbers).
#
# Uses the single evaluation harness (eval/). The benchmarks/<cond> folders are
# already degraded images, so we run them as-is with the "original" variant.
set -euo pipefail
cd "$(dirname "$0")/.."

for cond in clean low_light motion_blur worst_case; do
  echo "############ $cond ############"
  python -m eval.run_eval --dataset "benchmarks/$cond" --variants original \
    --out "run/benchmarks/$cond"
  echo
done

echo "Results in run/benchmarks/. With a key set, these hold the measured numbers."
