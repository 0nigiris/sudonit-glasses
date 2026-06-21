# CODE_SIMPLIFICATION_REPORT.md

> Repository-wide code-health audit. Every C source, Python module, test, tool, build
> script, and config file was read and its usage traced across the tree. This is a **map
> of technical debt** — no code is changed here. Findings are categorized **SAFE NOW**,
> **AFTER HARDWARE**, **DO NOT TOUCH**, each with file, location, why, benefit, and risk.
>
> **Scope measured:** ~3,722 lines of C (firmware) + ~3,124 lines of Python
> (phone/protocol/tools/eval/simulator/tests). The codebase is small and mostly clean;
> the debt is concentrated in **duplicated tooling** (some introduced very recently) and
> a few **unused abstractions** waiting on hardware.

---

## Headline

The code is in good shape relative to the docs. Three real problems:

1. **A duplicated benchmark/degradation toolchain** — `eval/` (added last session) re-implements
   what `tools/ai_benchmark.py` and `tools/camera_degrade.py` already do. This is the single
   biggest cleanup, and it is honestly self-inflicted.
2. **A dead end-to-end scaffold** — `simulator/glasses_sim.py` is a Python re-implementation of
   the device loop that the real firmware host build (`device_interop`) now supersedes.
3. **Build source lists duplicated** across the host and ESP-IDF CMake files.

Everything else is minor or correctly deferred. Notably clean: `.gitignore` is healthy (no
committed build artifacts or `sdkconfig`), there are **no stale TODOs** (the only two TODOs are
legitimate post-hardware markers), and no unreachable code was found in the core loop.

---

## SAFE NOW — can be changed immediately

### S1 · Dead scaffold: `simulator/glasses_sim.py`
- **File / location:** `simulator/glasses_sim.py` (whole file, 100 lines) + `simulator/__init__.py`.
- **Why redundant:** It re-implements the V1 loop in Python (ping → "capture" a file → stream →
  receive AI + audio). The firmware host build `device_interop` (`firmware/src/app/main_interop.c`
  running the **real** `device.c`) now exercises that exact loop over the real protocol, and
  `tools/end_to_end_demo.py` covers the in-process variant. Nothing imports `glasses_sim`
  (verified: only its own docstring references it). It is a third copy of "the loop."
- **Benefit:** −100 LOC, −1 module; removes one of three parallel "the loop" implementations to
  keep in sync.
- **Risk:** Low. Standalone manual script, not in any test or CI path. (Archive instead of delete
  if you want to keep the demo around.)

### S2 · Duplicate image-degradation logic: `eval/degrade.py` ⟷ `tools/camera_degrade.py`
- **Files / locations:** `eval/degrade.py` (135 lines) and `tools/camera_degrade.py` (254 lines).
  Both downscale to 800×600 (`camera_degrade` calls it `SVGA`), blur, darken for low light, and
  JPEG-compress. Tested twice: `tests/test_camera_degrade.py` (99) and the degradation half of
  `tests/test_eval_harness.py` (152).
- **Why redundant:** Two independent implementations of the same camera-degradation concept,
  introduced at different times (`camera_degrade` earlier, `eval/degrade` last session).
- **Benefit:** Collapse to one module + one test. Removes ~135 LOC and a parallel test surface.
- **Risk:** Low–medium. Both are tested; pick the richer API and re-point callers
  (`tools/end_to_end_demo.py` imports `tools.camera_degrade`; `eval/run_eval.py` imports
  `eval.degrade`). Behavior of degradation presets differs slightly, so this is a *merge*, not a
  blind delete.

### S3 · Duplicate AI-benchmark logic: `eval/run_eval.py` ⟷ `tools/ai_benchmark.py`
- **Files / locations:** `eval/run_eval.py` (232) + `eval/stats.py` (61) vs `tools/ai_benchmark.py`
  (193). Both run the provider over a folder and record latency / input+output tokens / cost /
  median+p90 summary. `eval/` is a **superset** (adds JSON output, degradation variants, per-variant
  breakdown); `ai_benchmark` only adds CSV.
- **Why redundant:** `eval/run_eval.py` does everything `tools/ai_benchmark.py` does and more. The
  cost maths (`_cost`), the percentile helper, and the summary are re-implemented in both.
- **Benefit:** Retire `tools/ai_benchmark.py` (193) + `tests/test_ai_benchmark.py` (74) once `eval/`
  covers them, or have `eval` import the shared bits. ~267 LOC + a tool a contributor must choose
  between.
- **Risk:** Low–medium. CSV consumers (if any) would move to JSON. Both tested, so the merge is
  verifiable.

### S4 · Unused build manifest: `firmware/components/protocol/CMakeLists.txt`
- **File / location:** `firmware/components/protocol/CMakeLists.txt` (whole file). Its own comment
  says "Reserved for a future modular split."
- **Why redundant:** Neither build uses it — the host build (`firmware/CMakeLists.txt`) and the
  ESP-IDF `main` component both compile the protocol sources **directly** by relative path. The
  component manifest is dead today.
- **Benefit:** Either delete (−1 file) or, better, **adopt** it to fix S5 (single source of truth
  for protocol sources).
- **Risk:** Low. It is not on any active build path.

### S5 · Duplicated build source lists (host side)
- **Files / locations:** `firmware/CMakeLists.txt:42-46` lists the four protocol sources;
  `firmware/esp-idf/main/CMakeLists.txt:30-33` lists the same four; the shared core/config/device/
  provisioning sources are likewise enumerated in **both** CMake files.
- **Why redundant:** Adding or renaming a shared source means editing two build files (three if you
  count the unused component manifest in S4). Easy to forget one and break a build.
- **Benefit:** Maintenance — one place to list sources. Adopting the protocol component (S4) +
  `EXTRA_COMPONENT_DIRS` removes the protocol duplication immediately on the host side.
- **Risk:** Low for the host-side protocol-component adoption; the **ESP-IDF-side** consolidation
  is listed under AFTER HARDWARE (A3) so the IDF build can be revalidated on real silicon.

### S6 · Factual defect: README license contradiction
- **File / location:** `README.md` footer ("License: To be determined") vs the Apache-2.0
  `LICENSE` file in the repo root; README also says "Pre-Prototype."
- **Why redundant/wrong:** A direct self-contradiction (already documented in the archived
  `README_REWRITE_PLAN.md`). Not duplication but a correctness bug surfaced by the audit.
- **Benefit:** Removes a credibility defect at the repo front door.
- **Risk:** None (one-line factual fix). *(Listed because it is a concrete, safe code-adjacent fix;
  the full README rewrite is tracked separately in `OPEN_TASKS.md`.)*

---

## AFTER HARDWARE — correct, but wait for validation

### A1 · Unused HAL abstraction: the microphone
- **Files / locations:** `firmware/include/sudonit/hal/mic.h` (21), `src/hal/mock/mic_mock.c` (41,
  generates a 440 Hz sine), `src/hal/esp32/mic_esp.c` (23, stub). Usage traced: defined in the two
  backends, exercised only by `test/test_hal.c`, and "used" in production only as a logged name via
  `sd_mic_backend()` in `app_main.c`. **`device.c` never reads the mic** — the uplink loop captures
  an image only.
- **Why flagged:** ~85 LOC of an abstraction that nothing in the product path calls. It is premature.
- **Benefit if removed:** −~85 LOC and one HAL surface.
- **Risk:** Medium — a V1 voice-question feature would legitimately need mic input, so deleting now
  and re-adding later wastes work. **Decide once the voice-input path is designed on hardware.** Do
  not remove blindly.

### A2 · Second capture path + unused battery telemetry
- **Files / locations:** `firmware/src/app/device.c:30-59` (`sd_device_capture_cycle`, reads battery
  + captures + logs) vs `device.c:104-161` (`sd_device_run_uplink`, captures directly). Battery is
  read and logged but **never transmitted or used**. `capture_cycle` is only called by
  `main_host.c` (the `device_demo`) and `app_main.c`'s self-demo — not by the real uplink.
- **Why flagged:** Two capture code paths (mild duplication of `sd_camera_capture` handling) and a
  battery read whose value goes nowhere.
- **Benefit:** Could fold the demo path into the real one and clarify battery's role.
- **Risk:** Medium — battery/telemetry is a genuine V1 need on hardware (low-battery UX), so keep the
  HAL; just don't ship a dead demo path. Revisit when the device gains real telemetry.

### A3 · ESP-IDF build-list consolidation (the IDF half of S5)
- **File / location:** `firmware/esp-idf/main/CMakeLists.txt:9-44`.
- **Why flagged:** Consolidating shared-source lists touches the IDF build; safest to restructure
  when you can immediately reflash and confirm the board still boots.
- **Benefit:** Single source of truth for sources across both builds.
- **Risk:** Medium until hardware is in hand to revalidate.

### A4 · Stub/real driver `#ifdef` branches collapse
- **Files / locations:** `firmware/src/hal/esp32/camera_esp.c` (173, real driver behind
  `SUDONIT_CAMERA_DRIVER`, stub otherwise) and `audio_esp.c` (133, behind `SUDONIT_AUDIO_DRIVER`).
  Same for the two real TODOs: `battery_esp.c:16`, `mic_esp.c:20`.
- **Why flagged:** The dual stub/real structure is justified *now* (default build compiles with no
  drivers), but once drivers are validated the stub branches become dead code.
- **Benefit:** After validation, delete the stub `#else` branches (~80 LOC of dead paths) and drop
  the flags.
- **Risk:** Low after hardware, but **do not** collapse before the real drivers are proven — the
  stubs are what keeps the default build green today.

---

## DO NOT TOUCH — looks redundant, is load-bearing

### D1 · The protocol mirror (C ⟷ Python)
- **Files:** `protocol/framing.py` `messages.py` ⟷ `firmware/components/protocol/src/framing.c`
  `messages.c` `json.c` `sha256.c`. This is literal "duplicated protocol definitions," **by design**:
  two languages implementing one wire spec, kept byte-identical, each independently tested
  (`tests/test_framing.py`, `firmware/test/test_protocol.c`). It is the contract between glasses and
  phone. **Keep both.** (A spec-to-code generator would be over-engineering at this scale.)

### D2 · The HAL interface + mock/host/esp32 backends
- **Files:** `firmware/include/sudonit/hal/*.h` + `src/hal/{mock,host,esp32}/*`. The swappable-backend
  layering is what lets the entire firmware compile and test on a laptop with no silicon — the
  project's core engineering bet. It reads like over-abstraction; it is the opposite. Keep (the only
  unused member is the mic, A1).

### D3 · Full-turn tests at three layers
- **Files:** `tests/test_end_to_end_demo.py` (tool), `tests/test_server_e2e.py` (server),
  `tests/test_firmware_interop.py` (firmware C). They overlap in *what* they prove but each guards a
  **different layer**; together they localize a regression to tool vs. server vs. firmware. Cheap and
  worth keeping.

### D4 · Provider abstraction + stub
- **Files:** `phone/ai/provider.py`, `stub_provider.py`, `anthropic_provider.py`, `__init__.py`. The
  indirection delivers offline-by-default runs and vendor-neutrality (a stated product requirement).
  Keep.

---

## Estimated impact

| Metric | SAFE NOW | AFTER HARDWARE | Notes |
|---|---|---|---|
| **Lines removable** | ~450–550 | ~165 | SAFE: simulator (~100) + one degrade impl (~135) + `ai_benchmark` (~193) + its test (~74). AFTER: mic HAL (~85) + collapsed driver stubs (~80). |
| **Files removable** | 4–5 | 3 | SAFE: `glasses_sim.py`, one of (`eval/degrade.py` \| `camera_degrade.py`), `tools/ai_benchmark.py`, `tests/test_ai_benchmark.py`, maybe `components/protocol/CMakeLists.txt`. AFTER: `mic.h`, `mic_mock.c`, `mic_esp.c`. |
| **Files mergeable** | 2 merges | 1 | `eval/` ⟷ `tools/` (benchmark + degradation); + build-source-list unification (host now, IDF after hardware). |
| **Complexity reduction** | High in tooling | Low–med | Eliminates an entire duplicated benchmark/degradation toolchain and a third copy of "the loop"; firmware stays as-is. |
| **Maintenance reduction** | Medium | Medium | One degradation module + one benchmark to keep current; protocol sources listed once; fewer "which tool do I run?" decisions. |

**Totals:** roughly **600–700 lines (~9–10% of all code)** and **7–8 files** are removable/mergeable
across both phases, with **~500 LOC safe to remove now**. Almost all of it is *duplication* (much of
it recent), not core logic — the firmware and the phone pipeline themselves are lean.

**Biggest single win:** consolidate `eval/` and `tools/` into one benchmark + one degradation module
(S2 + S3). It removes the most duplication, and it is duplication this project created in the last
two tasks — cleaning it up now prevents it from calcifying.

---

## What is NOT a problem (audited and cleared)

- **Committed build artifacts:** none. `.gitignore` correctly excludes `firmware/build/`,
  `firmware/esp-idf/build/`, `sdkconfig`, `managed_components/`, `run/`, `__pycache__`, `*.wav`.
- **Stale TODOs:** none. The only two (`battery_esp.c:16`, `mic_esp.c:20`) are valid post-hardware
  markers.
- **Secrets:** none hardcoded; API key is read from the environment only.
- **Unreachable code:** none found in the core capture→protocol→AI→audio path.
