# OPEN_TASKS.md

> Single living backlog of every task you've given me that is **not fully finished**.
> Purpose (your words): new ideas keep arriving while old ones stay half-done — this file
> is the one place that tracks what's still open, so nothing gets lost.
>
> **How to read it:** items are grouped by status. Each has its origin, what's actually
> left, any blocker, and the next concrete action. When something is finished, move it to
> "Done" at the bottom. Keep this file honest.
>
> Last updated: 2026-06-22 (reconciled with branch `repo-simplification`; pre-hardware
> code execution started).

---

## 🟢 Execution queue — unfinished CODE tasks (in priority order)

This is the live queue. Every item produces code / tests / CI / automation — not prose.
Origin: the pre-hardware plan (now in `DEVELOPMENT.md`). Status updated as each lands.

| # | Task | Output | Blocked by HW? | Status |
|---|------|--------|----------------|--------|
| A | CI: host build + ctest + pytest | `.github/workflows/ci.yml` | no | ☐ |
| B | Protocol robustness/property tests | `firmware/test/test_robustness.c` | no | ☐ |
| C | CI: ESP-IDF compile-only job | job in `ci.yml` | no | ☐ |
| D | Wire `app_main → sd_device_run_uplink` behind a flag | `app_main.c` + `main/CMakeLists.txt` | no | ☐ |
| E | Latency/size instrumentation in the host loop | `device.c` / `main_interop.c` | no | ☐ |
| F | Build automation (one-command build+test) | `Makefile` | no | ☐ |
| G | Lint / static analysis in CI | job in `ci.yml` | no | ☐ |

Each item is committed separately and validated (host build + ctest + pytest green)
before the next. Progress is reported per batch.

---

## 🟢 Blocked on hardware (June 29 — not actionable yet)
Tracked so they're not forgotten, but correctly deferred (see `HARDWARE.md` +
`VALIDATION.md`):
- Validate camera/audio drivers on real OV5640 / MAX98357A; implement battery ADC + mic I2S.
- Prove Wi-Fi bring-up on silicon; brownout / power-stability test (#1 risk).
- Real latency / battery / thermal numbers; the first real captured image + the live demo turn.
- Run the eval harness against **real Claude** (needs `ANTHROPIC_API_KEY`) for real cost/quality
  numbers — harness is built (`eval/`), just needs the key.
- After validation: collapse the stub/real driver `#ifdef`s and decide the mic HAL's fate.

---

## 🗂️ Repo-state hygiene
- **Branches:** simplification + pre-hardware code lives on `repo-simplification`; the older
  `interop-test-harness` and `docs-consolidation` were folded into it. Open one PR for
  `repo-simplification` → `main` when ready (direct pushes to `main` are blocked by policy).

---

## ✅ Done (verified)
- **O2 · README rewrite / license fix** — license contradiction resolved on
  `repo-simplification` (README now "Apache License 2.0 — see LICENSE").
- **O3 · Docs consolidation (Phases 1–4)** — 71 → 22 active docs, `docs/` removed, no
  information lost. Committed `bd50924`.
- **O4 · Code-simplification cleanups** — all SAFE-NOW items (S1–S6) executed: dead
  `simulator/` removed, `eval/`↔`tools/` duplication collapsed, unused protocol manifest
  dropped. Committed `7b466bf`. See `SIMPLIFICATION_EXECUTION_LOG.md`.
- **P1 · Automated interop test** — built and in-tree (`tests/test_firmware_interop.py`,
  `firmware/test/run_interop.sh`).
- Claude evaluation harness → `eval/` + `tests/test_eval_harness.py`, verified offline.
- Founder-vision alignment, demo package, launch-readiness, website review → in `archive/`.
