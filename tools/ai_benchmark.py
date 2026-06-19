"""AI_BENCHMARK_SUITE — replace guesses with numbers.

Runs every image in a folder through the *real* AI provider path
(phone.ai.get_provider — the same code the phone server uses) and records, per
image: wall-clock latency, input/output tokens, cost, and the answer. Emits a
CSV plus a summary (median/p90 latency, total/average cost, failure count).

Cost is computed from token usage and a configurable price table — never a
hardcoded total. With no ANTHROPIC_API_KEY the offline stub runs instead: you
still get latency and a valid CSV, but tokens/cost are blank (the stub cannot
measure them) — so a real cost run requires a real key, by design.

Usage:
    # real numbers (needs a key)
    ANTHROPIC_API_KEY=... python tools/ai_benchmark.py images/ --out run/bench.csv

    # offline smoke run (stub): structure + latency only
    python tools/ai_benchmark.py images/ --out run/bench.csv

    # override list prices (USD per 1M tokens) if they differ
    python tools/ai_benchmark.py images/ --price-in 15 --price-out 75

The default prices are a documented *estimate* for the Opus tier and are printed
on every run; pass --price-in/--price-out to pin them to the current rate card.
"""

from __future__ import annotations

import argparse
import csv
import statistics
import sys
import time
from dataclasses import dataclass
from pathlib import Path

# Run from the repo root so `phone` imports resolve.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from phone.ai import get_provider  # noqa: E402
from phone.ai.provider import DEFAULT_PROMPT  # noqa: E402

IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".webp", ".bmp"}
MEDIA_TYPES = {".png": "image/png", ".jpg": "image/jpeg", ".jpeg": "image/jpeg"}

# List-price ESTIMATE in USD per 1,000,000 tokens for the Opus tier. These are
# overridable on the CLI and printed on every run so the number is never silently
# wrong — pin them to the current rate card with --price-in/--price-out.
DEFAULT_PRICE_IN_PER_MTOK = 15.0
DEFAULT_PRICE_OUT_PER_MTOK = 75.0


@dataclass
class Row:
    image_name: str
    latency_ms: int
    input_tokens: int | None
    output_tokens: int | None
    cost: float | None
    response: str
    ok: bool


def _media_type(path: Path) -> str:
    return MEDIA_TYPES.get(path.suffix.lower(), "application/octet-stream")


def _cost(input_tokens, output_tokens, price_in, price_out) -> float | None:
    if input_tokens is None or output_tokens is None:
        return None
    return (input_tokens / 1e6) * price_in + (output_tokens / 1e6) * price_out


def run_benchmark(
    image_dir: Path,
    price_in: float,
    price_out: float,
    prompt: str = DEFAULT_PROMPT,
) -> list[Row]:
    provider = get_provider()
    print(f"[bench] provider: {provider.name}")
    images = sorted(
        p for p in image_dir.iterdir() if p.suffix.lower() in IMAGE_EXTS
    )
    if not images:
        raise SystemExit(f"no images found in {image_dir}")

    rows: list[Row] = []
    for path in images:
        data = path.read_bytes()
        started = time.monotonic()
        try:
            result = provider.analyze_image(data, _media_type(path), prompt)
            latency_ms = int((time.monotonic() - started) * 1000)
            cost = _cost(result.input_tokens, result.output_tokens, price_in, price_out)
            rows.append(
                Row(
                    image_name=path.name,
                    latency_ms=latency_ms,
                    input_tokens=result.input_tokens,
                    output_tokens=result.output_tokens,
                    cost=cost,
                    response=result.text,
                    ok=True,
                )
            )
            print(f"[bench] {path.name}: {latency_ms} ms"
                  + (f", ${cost:.4f}" if cost is not None else ", cost n/a"))
        except Exception as exc:  # a real run will hit refusals/timeouts/quota
            latency_ms = int((time.monotonic() - started) * 1000)
            rows.append(
                Row(path.name, latency_ms, None, None, None, f"ERROR: {exc}", False)
            )
            print(f"[bench] {path.name}: FAILED after {latency_ms} ms: {exc}")
    return rows


def write_csv(rows: list[Row], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(
            ["image_name", "latency_ms", "input_tokens", "output_tokens", "cost",
             "response"]
        )
        for r in rows:
            w.writerow([
                r.image_name,
                r.latency_ms,
                "" if r.input_tokens is None else r.input_tokens,
                "" if r.output_tokens is None else r.output_tokens,
                "" if r.cost is None else f"{r.cost:.6f}",
                r.response,
            ])


def summarize(rows: list[Row]) -> dict:
    ok = [r for r in rows if r.ok]
    latencies = sorted(r.latency_ms for r in ok)
    costs = [r.cost for r in ok if r.cost is not None]

    def pct(values, p):
        if not values:
            return None
        k = max(0, min(len(values) - 1, int(round((p / 100) * (len(values) - 1)))))
        return values[k]

    return {
        "images": len(rows),
        "failures": sum(1 for r in rows if not r.ok),
        "median_latency_ms": statistics.median(latencies) if latencies else None,
        "p90_latency_ms": pct(latencies, 90),
        "total_cost": round(sum(costs), 6) if costs else None,
        "avg_cost": round(sum(costs) / len(costs), 6) if costs else None,
    }


def print_summary(summary: dict, price_in: float, price_out: float) -> None:
    print("\n=== AI BENCHMARK SUMMARY ===")
    print(f"prices (USD/Mtok): input={price_in}  output={price_out}  (estimate; override with --price-in/--price-out)")
    print(f"images:           {summary['images']}")
    print(f"failures:         {summary['failures']}")
    print(f"median latency:   {summary['median_latency_ms']} ms")
    print(f"p90 latency:      {summary['p90_latency_ms']} ms")
    tc = summary["total_cost"]
    ac = summary["avg_cost"]
    print(f"total cost:       {'n/a (stub — set ANTHROPIC_API_KEY)' if tc is None else f'${tc:.4f}'}")
    print(f"avg cost/image:   {'n/a' if ac is None else f'${ac:.4f}'}")
    # The four product questions this run answers, restated against the numbers.
    if summary["median_latency_ms"] is not None:
        verdict = "feels instant" if summary["median_latency_ms"] <= 2000 else (
            "too slow — loses the phone comparison" if summary["median_latency_ms"] > 4000
            else "borderline")
        print(f"latency verdict:  {verdict} (target <=2000 ms median)")


def main() -> None:
    p = argparse.ArgumentParser(description="Benchmark the AI provider over a folder of images.")
    p.add_argument("image_dir", type=Path)
    p.add_argument("--out", type=Path, default=Path("run/ai_benchmark.csv"))
    p.add_argument("--price-in", type=float, default=DEFAULT_PRICE_IN_PER_MTOK)
    p.add_argument("--price-out", type=float, default=DEFAULT_PRICE_OUT_PER_MTOK)
    args = p.parse_args()

    rows = run_benchmark(args.image_dir, args.price_in, args.price_out)
    write_csv(rows, args.out)
    summary = summarize(rows)
    print_summary(summary, args.price_in, args.price_out)
    print(f"\n[bench] wrote {args.out} ({len(rows)} rows)")


if __name__ == "__main__":
    main()
