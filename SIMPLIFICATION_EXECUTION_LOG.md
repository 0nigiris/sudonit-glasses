# SIMPLIFICATION_EXECUTION_LOG.md

> Execution record for the repository simplification (driven by
> `CODE_SIMPLIFICATION_REPORT.md` + `DOCS_CONSOLIDATION_PLAN.md`). Every batch was
> validated — host build, `ctest`, `pytest` — and kept green before moving on. No
> behavior change to the product loop; reductions are dead code, duplicated tooling,
> and duplicated tests.

**Baseline (before any change), verified green:**
- Firmware host build: **OK**
- `ctest`: **4/4 passed**
- `pytest`: **53 passed**
- Code size: **~3,722 lines C/H**, **~3,124 lines Python**

---

## Phase 1 — SAFE NOW items (from `CODE_SIMPLIFICATION_REPORT.md`)

### Batch 1 — dead code / unused files / README fix
| Action | File(s) | Why | Reduction |
|---|---|---|---|
| Delete | `simulator/glasses_sim.py`, `simulator/__init__.py` | Dead scaffold — nothing imports it; the firmware host loop `device_interop` (real `device.c`) supersedes this Python re-implementation of the loop (report **S1**) | −101 LOC, −1 dir |
| Delete | `firmware/components/protocol/CMakeLists.txt` | Unused build manifest; neither build references `firmware/components` as a component dir — both compile the protocol sources directly (report **S4**). Verified no `EXTRA_COMPONENT_DIRS` hit | −1 file |
| Delete | `eval/dataset/*.jpg` (4 image copies) | Duplicated the `benchmarks/_source` images; `eval/run_eval` now defaults its dataset to `benchmarks/_source` | −4 dup binaries, −1 dir |
| Edit | `README.md` | Fixed license contradiction: "To be determined" → "Apache License 2.0 — see LICENSE" (report **S6**) | factual fix |
| Edit | `RUNNING.md`, `tools/make_sample_image.py`, `.gitignore` | Re-pointed the removed simulator's documented flow to `device_interop`; default sample path → `run/sample.png`; dropped the dead `simulator/sample` ignore | consistency |

**Validation:** build OK · ctest 4/4 · **pytest 53** (unchanged — simulator had no tests).

### Batch 2 — duplicate AI-benchmark tool (report **S3**)
| Action | File(s) | Why | Reduction |
|---|---|---|---|
| Re-point | `tools/run_benchmarks.sh` | Now calls the single eval harness (`python -m eval.run_eval`) instead of the duplicate tool | capability preserved |
| Delete | `tools/ai_benchmark.py` | Superseded by `eval/run_eval.py` (a strict superset: JSON, per-variant, degradation) | −193 LOC |
| Delete | `tests/test_ai_benchmark.py` | Tested only the deleted tool; eval is covered by `tests/test_eval_harness.py` | −74 LOC, −6 tests |

**Validation:** **pytest 47** (53 − 6 removed duplicate tests), no code refs to `ai_benchmark` remain.

### Batch 3 — duplicate degradation engine (report **S2**)
| Action | File(s) | Why | Reduction |
|---|---|---|---|
| Refactor | `eval/run_eval.py` | Now renders degradation via the shared `tools/camera_degrade.py` engine (one degradation implementation for the repo) — variants are now `original` + the `camera_degrade` presets | +~25 LOC |
| Delete | `eval/degrade.py` | Re-implemented what `tools/camera_degrade.py` already does (800×600, blur, low light, JPEG) | −135 LOC |
| Trim | `tests/test_eval_harness.py` | Removed the degradation-specific tests (degradation is tested once now, in `tests/test_camera_degrade.py`); kept the stats + runner tests | −14 tests, −~50 LOC |

**Validation:** live `eval` run OK (12 requests via the unified engine, 0 failures) · **pytest 33** (47 − 14 duplicate degradation tests) · ctest 4/4 · build OK.

### Phase 1 result
- **Files removed:** 7 (`simulator/glasses_sim.py`, `simulator/__init__.py`,
  `firmware/components/protocol/CMakeLists.txt`, `tools/ai_benchmark.py`,
  `tests/test_ai_benchmark.py`, `eval/degrade.py`, + the `eval/dataset/` image copies).
- **Tools merged:** 2 (AI-benchmark → `eval/`; degradation → `tools/camera_degrade.py`).
- **Duplicate tests removed:** 20 (6 ai_benchmark + 14 eval degradation), all still covered elsewhere.
- **Lines removed:** ~600 net Python (**3,124 → 2,524**); C/H unchanged (only a non-code CMake file removed).
- **Behavior:** product loop unchanged; only dev tooling consolidated. All suites green.
- **Deferred (correctly):** report **S5** (unify the host vs ESP-IDF build source lists) stays for
  AFTER HARDWARE — it touches the ESP-IDF build, which can't be validated without the toolchain/board.

---

## Phase 2 — Documentation consolidation (`DOCS_CONSOLIDATION_PLAN.md`)

Method: each source file was concatenated into its target under a labeled
`<!-- consolidated from: X -->` divider — **content preserved verbatim, nothing
dropped** — then the source removed. (Phase 1 of the plan, archiving 21 historical
files, was already done on the `docs-consolidation` branch this builds on.)

**New consolidated docs created (from merged sources):**
- `DEVELOPMENT.md` ← RUNNING, TEST_PLAN, IMPLEMENTATION_GAP_REPORT, PRE_HARDWARE_EXECUTION_PLAN, PRE-HARDWARE_STOP_LIST
- `HARDWARE.md` ← HARDWARE_ARRIVAL_CHECKLIST, HARDWARE_INTEGRATION_PLAN, BOARD_RESOURCES, CAMERA_BRINGUP, AUDIO_BRINGUP, PROVISIONING_PLAN, REFERENCES
- `VALIDATION.md` ← ASSUMPTION_REGISTER, VALIDATION_BACKLOG, TRUTH_TABLE, RISKS, FAILURE_SCENARIOS, FIRST_REAL_DATA_PLAN, DEMO_METRICS, DEMO_SUCCESS_CRITERIA
- `DEMO_AND_LAUNCH.md` ← DEMO_SCRIPT, DEMO_VIDEO_PLAN, DEMO_DAY_PLAN, PHOTO_SHOTLIST, LAUNCH_READINESS, FIRST_PUBLIC_POSTS

**Merged into existing canonical docs:** MASTER → `VISION.md`; APP_MODEL + FRAME_ARCHITECTURE → `ARCHITECTURE.md`; MILESTONES + PRE_HARDWARE_ROADMAP → `ROADMAP.md`; CLOUD → `PRODUCT.md`.

**Renamed:** `PROJECT_BRIEF_SUDONIT.md`→`PRODUCT.md`, `WEBSITE_CONTENT_MASTER.md`→`WEBSITE.md`, `docs/MVP_DEFINITION.md`→`MVP.md`.

**Deleted (pure duplicates, content already in canonical docs):** `docs/PRODUCT_VISION.md`, `TOP_RISKS.md`, `QUICK_WINS.md`, `CODE_FIRST_BACKLOG.md`.

**Other:** the entire `docs/` directory was emptied by the merges and removed;
`CONTRIBUTING.md`'s reading list re-pointed (MASTER→VISION, RUNNING→DEVELOPMENT).

**Result:** active project Markdown **71 → 22** (19 at root + 3 component-scoped),
with 21 historical files in `archive/`. The 13 "read these" core docs (README, VISION,
PRODUCT, ARCHITECTURE, MVP, DEVELOPMENT, HARDWARE, VALIDATION, DEMO_AND_LAUNCH, WEBSITE,
PROTOCOL, DECISIONS, ROADMAP) now cover the project. Content verified preserved
(consolidated docs are 500–1,400 lines each). Code untouched — build + ctest + pytest
still green.

## Phase 3 — Repository structure cleanup
- **`docs/` removed** — its 14 files were merged up into the root core docs (Phase 2).
- **`simulator/` removed** — dead, superseded by the firmware host loop (Phase 1).
- **`eval/` vs `tools/`** — both hold dev tooling but serve distinct roles: `tools/` is
  standalone scripts (`camera_degrade`, `end_to_end_demo`, `make_sample_image`,
  `run_benchmarks.sh`), `eval/` is the evaluation harness with its own output layout.
  Their **duplicated logic** (benchmark + degradation) was already collapsed in Phase 1
  so only one implementation of each remains; keeping the two directories is the clearer
  split, not duplication. No further merge.
- **`benchmarks/`** (dataset) and **`run/`** (gitignored output) are single-purpose; kept.

Resulting top-level layout is flat and non-overlapping: `firmware/`, `phone/`,
`protocol/`, `eval/`, `tools/`, `tests/`, `benchmarks/`, `archive/`.

## Phase 4 — Technical-debt cleanup
Re-reviewed every source file (the line-by-line pass is in `CODE_SIMPLIFICATION_REPORT.md`).
All **SAFE NOW** debt was removed in Phase 1. The remaining identified debt is
**correctly deferred to AFTER HARDWARE** and must not be removed yet without breaking
the build or pre-empting a planned V1 capability:
- **mic HAL** (`mic.h`/`mic_mock.c`/`mic_esp.c`) — unused by the loop today, but a planned
  V1 voice-input path; removing now risks re-adding later.
- **stub/real driver `#ifdef` branches** (`camera_esp.c`, `audio_esp.c`) — the stub
  branches keep the default build green; collapse only once real drivers are validated.
- **ESP-IDF build source-list duplication** — touches a build that can't be validated
  without the toolchain/board.

No unnecessary abstraction, function, or layer was found that is safe to remove now
beyond what Phase 1 already did. Deleting any of the above today would break a build or
remove a load-bearing layer, violating the "never break working code" rule.

## Final tally
| Metric | Result |
|---|---|
| Files removed (code) | 7 |
| Files removed/merged (docs) | 71 → 22 active (49 fewer; 21 archived, rest merged/deleted) |
| Directories removed | 2 (`docs/`, `simulator/`) |
| Python lines removed | ~600 (3,124 → 2,524) |
| Duplicate tests removed | 20 (all still covered elsewhere) |
| Build / ctest / pytest | green at every step (4/4 ctest, 33 pytest) |
| Product-loop behavior | unchanged |
