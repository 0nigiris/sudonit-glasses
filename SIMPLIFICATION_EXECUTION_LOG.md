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

## Phase 2 — Documentation consolidation
_(in progress — see progress entries below)_

## Phase 3 — Repository structure cleanup
_(pending)_

## Phase 4 — Technical-debt cleanup
_(pending)_
