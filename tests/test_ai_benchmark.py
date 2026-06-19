"""Tests for the AI benchmark suite (runs on the offline stub provider)."""

from __future__ import annotations

import csv

import pytest

from tools.ai_benchmark import run_benchmark, summarize, write_csv
from tools.make_sample_image import make_png


@pytest.fixture(autouse=True)
def force_stub(monkeypatch):
    # Deterministic, offline, no key/cost — the benchmark structure is what we test.
    monkeypatch.setenv("SUDONIT_AI_PROVIDER", "stub")


def _image_dir(tmp_path, n=3):
    for i in range(n):
        make_png(tmp_path / f"img_{i}.png")
    return tmp_path


def test_one_row_per_image(tmp_path):
    rows = run_benchmark(_image_dir(tmp_path, 3), price_in=15.0, price_out=75.0)
    assert len(rows) == 3
    assert all(r.ok for r in rows)
    assert all(isinstance(r.latency_ms, int) and r.latency_ms >= 0 for r in rows)


def test_stub_reports_no_tokens_or_cost(tmp_path):
    rows = run_benchmark(_image_dir(tmp_path, 2), price_in=15.0, price_out=75.0)
    # The stub can't measure tokens; cost must be None (not a fabricated zero).
    assert all(r.input_tokens is None and r.cost is None for r in rows)


def test_summary_fields(tmp_path):
    rows = run_benchmark(_image_dir(tmp_path, 4), price_in=15.0, price_out=75.0)
    s = summarize(rows)
    assert s["images"] == 4
    assert s["failures"] == 0
    assert s["median_latency_ms"] is not None
    assert s["p90_latency_ms"] is not None
    # No cost on the stub path.
    assert s["total_cost"] is None and s["avg_cost"] is None


def test_csv_has_required_columns(tmp_path):
    rows = run_benchmark(_image_dir(tmp_path, 2), price_in=15.0, price_out=75.0)
    out = tmp_path / "bench.csv"
    write_csv(rows, out)
    with out.open(newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        header = next(reader)
        body = list(reader)
    assert header == [
        "image_name", "latency_ms", "input_tokens", "output_tokens", "cost", "response",
    ]
    assert len(body) == 2


def test_cost_math_when_tokens_present():
    # Directly exercise the cost helper (the real-provider path computes this).
    from tools.ai_benchmark import _cost

    # 1000 in @ $15/M + 500 out @ $75/M = 0.015 + 0.0375 = 0.0525
    assert _cost(1000, 500, 15.0, 75.0) == pytest.approx(0.0525)
    assert _cost(None, 500, 15.0, 75.0) is None  # unknown tokens -> unknown cost


def test_empty_dir_raises(tmp_path):
    with pytest.raises(SystemExit):
        run_benchmark(tmp_path, price_in=15.0, price_out=75.0)
