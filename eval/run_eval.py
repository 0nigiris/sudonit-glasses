"""CLI runner for the Claude evaluation harness.

Runs the real AI provider path over a dataset of images — each image in one or
more degradation variants (see tools/camera_degrade.py) — and records, per request:
latency, input/output tokens, cost, and the answer. Writes a timestamped run
under the benchmark directory as JSON, and prints median/p90 latency and average
cost.

Usage:
    # real numbers (needs a key); all variants of every image in benchmarks/_source
    ANTHROPIC_API_KEY=... python -m eval.run_eval

    # offline smoke run (stub provider): structure + latency only, no cost
    python -m eval.run_eval

    # choose variants, dataset, and output location
    python -m eval.run_eval --dataset benchmarks/_source --out eval/benchmark \
        --variants original,resize_800x600,jpeg,blur,low_light,combined

    # pin prices to the current rate card (USD per 1M tokens)
    python -m eval.run_eval --price-in 15 --price-out 75

Cost is derived from real reported token usage and a printed, overridable price
table — never a hardcoded total. With the stub provider tokens/cost are null.
"""

from __future__ import annotations

import argparse
import io
import json
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

# Run as a module (python -m eval.run_eval) or a script; make `phone`/`eval`
# imports resolve from the repo root either way.
REPO_ROOT = Path(__file__).resolve().parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from PIL import Image  # noqa: E402

from eval.stats import summarize  # noqa: E402
from phone.ai import get_provider  # noqa: E402
from phone.ai.provider import DEFAULT_PROMPT  # noqa: E402
from tools.camera_degrade import PRESETS, degrade as _degrade  # noqa: E402

# Default dataset: the shared source images the benchmarks/ dataset is built from.
DEFAULT_DATASET = REPO_ROOT / "benchmarks" / "_source"
DEFAULT_OUT = REPO_ROOT / "eval" / "benchmark"

_MEDIA_BY_SUFFIX = {".png": "image/png", ".jpg": "image/jpeg", ".jpeg": "image/jpeg",
                    ".webp": "image/webp", ".bmp": "image/bmp"}

# Degradation variants: "original" (untouched) plus the shared camera-degradation
# presets from tools/camera_degrade.py — one degradation engine for the whole repo.
VARIANTS = ("original", *PRESETS.keys())


def _render(raw: bytes, variant: str, suffix: str) -> tuple[bytes, str, dict]:
    """Render one variant of an image. 'original' passes the bytes through; any
    other variant runs the shared camera-degradation engine and re-encodes JPEG."""
    if variant == "original":
        mt = _MEDIA_BY_SUFFIX.get(suffix.lower(), "application/octet-stream")
        return raw, mt, {"variant": "original"}
    decoded, used = _degrade(Image.open(io.BytesIO(raw)), PRESETS[variant])
    buf = io.BytesIO()
    decoded.save(buf, format="JPEG", quality=used["jpeg_quality"])
    return buf.getvalue(), "image/jpeg", {"variant": variant, **used}

# List-price ESTIMATE in USD per 1,000,000 tokens (Opus tier). Overridable and
# printed every run so the figure is never silently wrong.
DEFAULT_PRICE_IN_PER_MTOK = 15.0
DEFAULT_PRICE_OUT_PER_MTOK = 75.0

IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".webp", ".bmp"}


def _cost(input_tokens, output_tokens, price_in, price_out) -> float | None:
    if input_tokens is None or output_tokens is None:
        return None
    return (input_tokens / 1e6) * price_in + (output_tokens / 1e6) * price_out


def discover_images(dataset: Path) -> list[Path]:
    if not dataset.is_dir():
        raise SystemExit(f"dataset directory not found: {dataset}")
    images = sorted(p for p in dataset.iterdir() if p.suffix.lower() in IMAGE_EXTS)
    if not images:
        raise SystemExit(f"no images found in {dataset} "
                         f"(supported: {', '.join(sorted(IMAGE_EXTS))})")
    return images


def run(
    dataset: Path,
    variants: list[str],
    price_in: float,
    price_out: float,
    prompt: str = DEFAULT_PROMPT,
    provider_name: str | None = None,
    limit: int | None = None,
) -> list[dict]:
    """Evaluate every (image, variant) pair and return a list of record dicts."""
    provider = get_provider(provider_name)
    images = discover_images(dataset)
    if limit is not None:
        images = images[:limit]

    print(f"[eval] provider: {provider.name}")
    print(f"[eval] dataset:  {dataset} ({len(images)} images)")
    print(f"[eval] variants: {', '.join(variants)}")

    records: list[dict] = []
    for path in images:
        raw = path.read_bytes()
        for variant in variants:
            data, media_type, meta = _render(raw, variant, path.suffix)
            started = time.monotonic()
            try:
                result = provider.analyze_image(data, media_type, prompt)
                latency_ms = int((time.monotonic() - started) * 1000)
                cost = _cost(result.input_tokens, result.output_tokens,
                             price_in, price_out)
                records.append({
                    "image": path.name,
                    "variant": variant,
                    "ok": True,
                    "latency_ms": latency_ms,
                    "input_tokens": result.input_tokens,
                    "output_tokens": result.output_tokens,
                    "cost": cost,
                    "media_type": media_type,
                    "bytes": len(data),
                    "model": result.model,
                    "answer": result.text,
                })
                tag = f"${cost:.4f}" if cost is not None else "cost n/a"
                print(f"[eval] {path.name} [{variant}]: {latency_ms} ms, {tag}")
            except Exception as exc:  # refusals/timeouts/quota on a real run
                latency_ms = int((time.monotonic() - started) * 1000)
                records.append({
                    "image": path.name,
                    "variant": variant,
                    "ok": False,
                    "latency_ms": latency_ms,
                    "input_tokens": None,
                    "output_tokens": None,
                    "cost": None,
                    "media_type": media_type,
                    "bytes": len(data),
                    "model": None,
                    "answer": f"ERROR: {exc}",
                })
                print(f"[eval] {path.name} [{variant}]: FAILED after {latency_ms} ms: {exc}")
    return records


def write_run(
    out_dir: Path,
    records: list[dict],
    summary: dict,
    meta: dict,
) -> Path:
    """Persist one run as JSON. Returns the run directory."""
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    run_dir = out_dir / f"run_{stamp}"
    run_dir.mkdir(parents=True, exist_ok=True)

    payload = {"meta": meta, "summary": summary, "records": records}
    (run_dir / "results.json").write_text(
        json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8"
    )
    (run_dir / "summary.json").write_text(
        json.dumps({"meta": meta, "summary": summary}, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    # The answer log: just image/variant -> answer, for fast human review.
    answers = [
        {"image": r["image"], "variant": r["variant"], "ok": r["ok"],
         "answer": r["answer"]}
        for r in records
    ]
    (run_dir / "answers.json").write_text(
        json.dumps(answers, indent=2, ensure_ascii=False), encoding="utf-8"
    )
    return run_dir


def print_summary(summary: dict, price_in: float, price_out: float) -> None:
    print("\n=== AI EVALUATION SUMMARY ===")
    print(f"prices (USD/Mtok): input={price_in} output={price_out} "
          f"(estimate; override with --price-in/--price-out)")
    print(f"requests:        {summary['requests']}")
    print(f"failures:        {summary['failures']}")
    print(f"median latency:  {summary['median_latency_ms']} ms")
    print(f"p90 latency:     {summary['p90_latency_ms']} ms")
    ac = summary["avg_cost"]
    print(f"avg cost/req:    {'n/a (stub — set ANTHROPIC_API_KEY)' if ac is None else f'${ac:.4f}'}")

    if summary["by_variant"]:
        print("\nper-variant (median latency / avg cost):")
        for variant, blk in summary["by_variant"].items():
            cost = blk["avg_cost"]
            cost_s = "n/a" if cost is None else f"${cost:.4f}"
            print(f"  {variant:<16} {str(blk['median_latency_ms']) + ' ms':<10} {cost_s}"
                  + (f"  ({blk['failures']} failed)" if blk["failures"] else ""))


def main(argv: list[str] | None = None) -> Path:
    p = argparse.ArgumentParser(
        description="Evaluate the Sudonit AI provider over a dataset of images.")
    p.add_argument("--dataset", type=Path, default=DEFAULT_DATASET,
                   help="folder of input images (default: benchmarks/_source)")
    p.add_argument("--out", type=Path, default=DEFAULT_OUT,
                   help="benchmark output directory (default: eval/benchmark)")
    p.add_argument("--variants", type=str, default=",".join(VARIANTS),
                   help="comma-separated degradation variants "
                        f"(known: {', '.join(VARIANTS)})")
    p.add_argument("--provider", type=str, default=None,
                   help="force a provider: anthropic | stub (default: auto)")
    p.add_argument("--price-in", type=float, default=DEFAULT_PRICE_IN_PER_MTOK)
    p.add_argument("--price-out", type=float, default=DEFAULT_PRICE_OUT_PER_MTOK)
    p.add_argument("--limit", type=int, default=None,
                   help="evaluate at most N images (smoke runs)")
    p.add_argument("--prompt", type=str, default=DEFAULT_PROMPT)
    args = p.parse_args(argv)

    variants = [v.strip() for v in args.variants.split(",") if v.strip()]
    unknown = [v for v in variants if v not in VARIANTS]
    if unknown:
        raise SystemExit(f"unknown variant(s): {', '.join(unknown)} "
                         f"(known: {', '.join(VARIANTS)})")

    records = run(args.dataset, variants, args.price_in, args.price_out,
                  args.prompt, args.provider, args.limit)
    summary = summarize(records)
    meta = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "dataset": str(args.dataset),
        "variants": variants,
        "price_in_per_mtok": args.price_in,
        "price_out_per_mtok": args.price_out,
        "prompt": args.prompt,
    }
    run_dir = write_run(args.out, records, summary, meta)
    print_summary(summary, args.price_in, args.price_out)
    print(f"\n[eval] wrote {run_dir}/ (results.json, summary.json, answers.json)")
    return run_dir


if __name__ == "__main__":
    main()
