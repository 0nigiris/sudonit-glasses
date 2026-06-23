"""Tests for the Claude evaluation harness (eval/).

These run fully offline against the deterministic stub provider, so they prove
the harness — runner, JSON output, and summary maths — works on any machine with
no API key and no network. A real-Claude run reuses the exact same code path; only
the provider differs.

The degradation engine itself (tools/camera_degrade.py) is tested by
tests/test_camera_degrade.py; this file only checks how the harness uses it.
"""

from __future__ import annotations

import json
from pathlib import Path

from PIL import Image

from eval.run_eval import main, run
from eval.stats import percentile, summarize


def _make_image(path: Path, size=(1024, 768), color=(120, 60, 30)) -> Path:
    Image.new("RGB", size, color).save(path, format="JPEG", quality=90)
    return path


# --- summary statistics ---------------------------------------------------

def test_percentile_basic():
    assert percentile([], 90) is None
    assert percentile([5], 90) == 5
    # nearest-rank: round(0.9 * (10-1)) = round(8.1) = index 8 -> value 9
    assert percentile([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], 90) == 9
    assert percentile([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], 50) in (5, 6)


def test_summarize_groups_by_variant_and_counts_failures():
    records = [
        {"variant": "original", "ok": True, "latency_ms": 100, "cost": 0.01},
        {"variant": "original", "ok": True, "latency_ms": 300, "cost": 0.03},
        {"variant": "typical", "ok": True, "latency_ms": 200, "cost": None},
        {"variant": "typical", "ok": False, "latency_ms": 50, "cost": None},
    ]
    s = summarize(records)
    assert s["requests"] == 4
    assert s["failures"] == 1
    assert s["median_latency_ms"] == 200  # over the 3 ok records: 100,200,300
    assert s["p90_latency_ms"] == 300
    assert set(s["by_variant"]) == {"original", "typical"}
    assert s["by_variant"]["original"]["avg_cost"] == 0.02
    assert s["by_variant"]["typical"]["failures"] == 1
    assert s["by_variant"]["typical"]["avg_cost"] is None  # stub-like: cost unknown


# --- end-to-end runner (offline, stub provider) ---------------------------

def test_run_over_dataset_with_stub_records_every_pair(tmp_path):
    dataset = tmp_path / "dataset"
    dataset.mkdir()
    _make_image(dataset / "a.jpg")
    _make_image(dataset / "b.jpg", color=(30, 90, 160))

    variants = ["original", "low_light", "worst_case"]
    records = run(dataset, variants, price_in=15, price_out=75,
                  provider_name="stub")

    assert len(records) == 2 * len(variants)
    assert all(r["ok"] for r in records), "stub turns must all succeed"
    assert all(r["latency_ms"] >= 0 for r in records)
    # The stub cannot report tokens, so cost must be null (never fabricated).
    assert all(r["cost"] is None for r in records)
    assert all(r["answer"] for r in records)


def test_main_writes_json_artifacts(tmp_path):
    dataset = tmp_path / "dataset"
    dataset.mkdir()
    _make_image(dataset / "only.jpg")
    out = tmp_path / "benchmark"

    run_dir = main([
        "--dataset", str(dataset),
        "--out", str(out),
        "--variants", "original,typical",
        "--provider", "stub",
    ])

    for name in ("results.json", "summary.json", "answers.json"):
        assert (run_dir / name).exists(), f"missing {name}"

    results = json.loads((run_dir / "results.json").read_text())
    assert results["summary"]["requests"] == 2
    assert "median_latency_ms" in results["summary"]
    assert "p90_latency_ms" in results["summary"]
    assert set(results["summary"]["by_variant"]) == {"original", "typical"}

    answers = json.loads((run_dir / "answers.json").read_text())
    assert len(answers) == 2
    assert all("answer" in a for a in answers)
