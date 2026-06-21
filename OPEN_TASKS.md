# OPEN_TASKS.md

> Single living backlog of every task you've given me that is **not fully finished**.
> Purpose (your words): new ideas keep arriving while old ones stay half-done — this file
> is the one place that tracks what's still open, so nothing gets lost.
>
> **How to read it:** items are grouped by status and sorted by priority. Each has its
> origin, what's actually left, any blocker, and the next concrete action. When something
> is finished, move it to "Done" at the bottom (or delete it). Keep this file honest.
>
> Last updated: 2026-06-21.

---

## 🔴 Open — not started (highest value first)

### O1 · Execute the pre-hardware code plan (`PRE_HARDWARE_EXECUTION_PLAN.md`)
The *plan* exists; the **code does not**. Only task #6 (the eval harness) was built. Still open:
- **CI: host build + test workflow** — *(plan #1, the recommended "start immediately")*. No
  build/test CI exists yet; all tests only run by hand. **Highest ROI item in the repo.**
- **Protocol robustness / property tests** *(plan #2)* — truncated frames, bad SHA, partial chunks.
- **CI: ESP-IDF compile job** *(plan #3)* — `idf.py build` in CI (Docker), stub + flagged builds.
- **Wire `app_main.c` → `sd_device_run_uplink` behind a flag** *(plan #4)* — the "last useful
  pre-hardware task"; makes June 29 a flash-and-run day.
- **Degraded-image interop matrix test** *(plan #5)*.
- **Latency / size instrumentation in the host loop** *(plan #7)*.
- **Build automation script / Makefile** *(plan #8)*.
- **Static analysis / lint in CI** *(plan #9)*.
- **Next action:** start with the CI workflow (#1) — unblocks safe change everywhere.

### O2 · Apply the README rewrite (`archive/README_REWRITE_PLAN.md`)
The plan was written; the README was **never updated**. The license contradiction is still live
(`README.md` says "License: To be determined" next to an Apache-2.0 `LICENSE`), and it still says
"Pre-Prototype" with the outdated 6-phase roadmap.
- **Why it matters:** repo front door; flagged again as **S6** in `CODE_SIMPLIFICATION_REPORT.md`.
- **Next action:** rewrite `README.md` per the archived plan; pull wording from
  `PROJECT_BRIEF_SUDONIT.md`. Small, safe, do it before any public eyes.

### O3 · Docs consolidation Phases 2–4 (`DOCS_CONSOLIDATION_PLAN.md`)
Phase 1 (archive 21 historical files) is **done and pushed** on branch `docs-consolidation`. Still open:
- **Phase 2 — merge** 32 files into `DEVELOPMENT.md` / `HARDWARE.md` / `VALIDATION.md` /
  `DEMO_AND_LAUNCH.md` (one reviewed PR per target).
- **Phase 3 — delete** 4 pure duplicates (after Phase 2 verifies content landed).
- **Phase 4 — rename + relink** (`PROJECT_BRIEF_SUDONIT.md`→`PRODUCT.md`,
  `WEBSITE_CONTENT_MASTER.md`→`WEBSITE.md`, `docs/MVP_DEFINITION.md`→`MVP.md`).
- **Next action:** do one target (e.g. build `VALIDATION.md` from the risk/assumption cluster) as a
  sample merge for you to approve before the rest.

### O4 · Code-simplification cleanups (`CODE_SIMPLIFICATION_REPORT.md`)
The audit is done; the cleanups are **not executed** (report-only by request). SAFE-NOW items:
- **S2 + S3** — merge `eval/` and `tools/` into one benchmark + one degradation module (biggest win;
  duplication I introduced last session).
- **S1** — remove/archive the dead `simulator/glasses_sim.py`.
- **S4 + S5** — drop or adopt the unused protocol component manifest; unify the duplicated CMake
  source lists (host side now).
- **Next action:** start with S2+S3 (the self-inflicted duplication), one merge per commit.

---

## 🟡 In progress / partially done

### P1 · "Build, don't document — then repeat for the next artifact"
Implemented the **automated interop test** (committed `b4de018` on branch `interop-test-harness`,
pushed, never merged via PR). The "repeat for the next highest-value artifact" instruction was
partly continued by the eval harness, but the broader backlog is now tracked under **O1** above.
- **Next action:** open a PR for `interop-test-harness` (or fold it into the CI work in O1).

---

## 🟢 Blocked on hardware (June 29 — not actionable yet)
Tracked so they're not forgotten, but correctly deferred (see
`PRE_HARDWARE_EXECUTION_PLAN.md` "impossible until hardware" + `HARDWARE_ARRIVAL_CHECKLIST.md`):
- Validate camera/audio drivers on real OV5640 / MAX98357A; implement battery ADC + mic I2S.
- Prove Wi-Fi bring-up on silicon; brownout / power-stability test (#1 risk).
- Real latency / battery / thermal numbers; the first real captured image + the live demo turn.
- Run the eval harness against **real Claude** (needs `ANTHROPIC_API_KEY`) for real cost/quality
  numbers — harness is built (`eval/`), just needs the key.
- After validation: collapse the stub/real driver `#ifdef`s (report **A4**) and decide the mic
  HAL's fate (report **A1**).

---

## 🗂️ Repo-state hygiene (cuts across everything)
Not a "task you gave me," but it's why old work feels lost — a lot of finished deliverables are
**uncommitted or stranded on branches**:
- **Uncommitted in the working tree:** `LAUNCH_READINESS.md`, `DEMO_SCRIPT.md`, `DEMO_VIDEO_PLAN.md`,
  `PHOTO_SHOTLIST.md`, `FIRST_PUBLIC_POSTS.md`, `PRE_HARDWARE_EXECUTION_PLAN.md`, `eval/`,
  `benchmarks/`, `tools/`, `CODE_SIMPLIFICATION_REPORT.md`, this file, and more.
- **Stranded branches (never merged to `main`):** `interop-test-harness`, `docs-consolidation`.
- **`main` vs `origin/main`:** direct pushes to `main` are blocked by policy; everything goes via a
  branch/PR, so work accumulates unmerged.
- **Next action:** decide a commit/PR strategy and land the backlog — otherwise this tracker grows
  while the repo history stays empty of the work.

---

## ✅ Done (for reference — verify before relying on)
- Founder-vision alignment report → `archive/FOUNDER_VISION_ALIGNMENT.md`.
- Pre-hardware artifact ranking (build-don't-document) → delivered in chat.
- `PROJECT_BRIEF_SUDONIT.md`, `WEBSITE_CONTENT_MASTER.md` → committed (`9c3e6f4`).
- `WEBSITE_REVIEW.md` → written, now `archive/WEBSITE_REVIEW.md`.
- `LAUNCH_READINESS.md` → written *(uncommitted — see hygiene)*.
- First-demo package: `DEMO_SCRIPT.md`, `DEMO_VIDEO_PLAN.md`, `PHOTO_SHOTLIST.md`,
  `FIRST_PUBLIC_POSTS.md`, `README_REWRITE_PLAN.md` (archived) *(uncommitted)*.
- `PRE_HARDWARE_EXECUTION_PLAN.md` → written *(the plan; its code tasks are O1)*.
- Claude evaluation harness → `eval/` + `tests/test_eval_harness.py`, verified offline *(uncommitted)*.
- Docs consolidation **Phase 1** → `DOCS_CONSOLIDATION_PLAN.md` + `archive/` (committed + pushed on
  `docs-consolidation`).
- Repository deep audit → `CODE_SIMPLIFICATION_REPORT.md` *(this file's sibling)*.
