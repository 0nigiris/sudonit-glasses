"""Summary statistics for an evaluation run.

Kept dependency-free (stdlib only) and separated from the runner so the maths is
unit-testable on its own. A "record" is one dict produced by run_eval per request
(see run_eval.Record); only successful records contribute to latency/cost stats.
"""

from __future__ import annotations

import statistics
from typing import Iterable


def percentile(values: list[float], p: float) -> float | None:
    """Nearest-rank percentile of a list (p in 0..100). None if empty."""
    if not values:
        return None
    ordered = sorted(values)
    k = max(0, min(len(ordered) - 1, int(round((p / 100) * (len(ordered) - 1)))))
    return ordered[k]


def _latencies(records: Iterable[dict]) -> list[float]:
    return [r["latency_ms"] for r in records if r.get("ok")]


def _costs(records: Iterable[dict]) -> list[float]:
    return [r["cost"] for r in records if r.get("ok") and r.get("cost") is not None]


def _block(records: list[dict]) -> dict:
    """Latency/cost/count stats for one group of records."""
    lat = _latencies(records)
    costs = _costs(records)
    return {
        "requests": len(records),
        "failures": sum(1 for r in records if not r.get("ok")),
        "median_latency_ms": statistics.median(lat) if lat else None,
        "p90_latency_ms": percentile(lat, 90),
        "avg_latency_ms": round(statistics.mean(lat), 1) if lat else None,
        "avg_cost": round(statistics.mean(costs), 6) if costs else None,
        "total_cost": round(sum(costs), 6) if costs else None,
    }


def summarize(records: list[dict]) -> dict:
    """Overall summary plus a per-variant breakdown.

    The per-variant view is the point of the harness: it shows whether (and how
    much) latency/cost change as the image degrades, not just an aggregate.
    """
    overall = _block(records)

    by_variant: dict[str, dict] = {}
    for r in records:
        by_variant.setdefault(r.get("variant", "unknown"), []).append(r)

    overall["by_variant"] = {
        variant: _block(group) for variant, group in sorted(by_variant.items())
    }
    return overall
