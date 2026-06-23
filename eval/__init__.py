"""Claude evaluation harness for Sudonit.

Measures whether the AI part of the glasses is actually useful: runs the real
provider path (phone.ai.get_provider) over a dataset of images — originals and
camera-realistic degraded variants — and records latency, token usage, cost, and
the answer for every request. Results are written as JSON.

Layout:
  eval/benchmark/  output runs (results.json / summary.json / answers.json)
  eval/run_eval.py CLI runner
  eval/stats.py    summary statistics (median / p90 latency, average cost)

Degradation variants reuse the shared engine in tools/camera_degrade.py (one
degradation implementation for the whole repo). Input images default to the
shared benchmarks/_source set.

With ANTHROPIC_API_KEY set, this hits real Claude and produces real cost/token
numbers. Without a key it falls back to the offline stub: you still get latency
and valid JSON, but tokens/cost are null (the stub cannot measure them) — so real
cost numbers require a real key, by design. Numbers are never fabricated.
"""
