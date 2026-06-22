# OPEN_TASKS.md

> **The single source of truth for outstanding work.** Authority is this file —
> not memory, not prior reports, not assumptions. Every task carries Priority,
> Status, hardware-block flag, effort, and dependencies.
>
> Priority key (P0 highest):
> - **P0** — bugs, correctness, build failures, CI failures, hardware bring-up risks
> - **P1** — tests, automation, tooling, validation, benchmarks
> - **P2** — cleanup, simplification, dead-code removal, deduplication
> - **P3** — documentation polish
>
> Status: ☐ open · ▶ in progress · ✅ done (commit) · ⛔ blocked (hardware/env)
>
> Last updated: 2026-06-23 — red-team code review. Found + fixed one
> previously-missed **P0** (malformed-peer server crash, T9). Earlier passes
> fixed T1–T8. **All P0/P1/P2/P3 tasks now done; only hardware/on-target-toolchain
> items remain (table below).**

---

## P0 / P1 / P2 / P3 — all cleared ✅

Every non-hardware task is implemented, validated (build + ctest 5/5 + pytest 50 +
ruff + cppcheck all green), and committed separately. See **✅ Done** below for the
per-task commit hashes. No open P0/P1/P2/P3 work remains — the only outstanding
items need real silicon or a validated ESP-IDF toolchain.

---

## ⛔ Blocked — hardware / on-target toolchain (not actionable before June 29)

These need real silicon or a validated ESP-IDF build environment; do **not** change blind.

| Task | Why blocked | Priority when unblocked |
|------|-------------|-------------------------|
| **H1** · Real camera driver (OV5640) on `SUDONIT_CAMERA_DRIVER` | needs the sensor | P0 |
| **H2** · Real audio driver (MAX98357A I2S) on `SUDONIT_AUDIO_DRIVER` | needs the amp | P0 |
| **H3** · Battery ADC (`battery_esp.c:16`) + mic I2S (`mic_esp.c:20`) | needs the board | P1 |
| **H4** · Wi-Fi bring-up + brownout / power-stability test (#1 risk) | needs silicon | P0 |
| **H5** · First real latency / battery / thermal numbers via the instrumented loop | needs silicon | P1 |
| **H6** · Run eval harness against real Claude (`ANTHROPIC_API_KEY`) for real cost/quality | needs key (not HW) | P1 |
| **H7** · ESP-IDF CI determinism: commit `dependencies.lock` + pin `esp32-camera` (manifest says "pin once verified on target") and confirm `idf.py build` is green in the container | needs a validated IDF toolchain; the job re-resolves `*` + recompiles esp32-camera every run today | P1 |
| **H8** · After driver validation: collapse stub/real `#ifdef` branches (report A4), fold the dead demo capture path (A2), decide mic-HAL fate (A1), unify the IDF source list (A3) | removing stubs before drivers are proven breaks the default build | P2 |

### Known unresolved risks (carried, not yet failures)
- **Framing desync on oversize (latent, not currently reachable).** `sd_frame_recv`
  returns `SD_ERR_NO_MEM` when `len > cap` *after* reading the 5-byte header but
  *without* draining the `len` payload bytes — leaving the stream frame-misaligned.
  Inspected and deliberately left unchanged: every caller (`sd_msg_recv` →
  `sd_device_run_uplink`) treats any recv error as fatal and stops reading, and a
  connection runs exactly one turn before close (`main_interop.c`, `app_main.c`), so
  no caller ever reads after the error. It becomes a real bug only if a future caller
  tries to "skip an oversized frame and continue" (e.g. recovering from a >8 KB
  `ai_response`) — fix then is to drain `len` bytes before returning, with a stream-
  recovery test. Not done now to avoid behavior change + complexity with no trigger.
- **`ai_response` > 8 KB** still exceeds the (heap, post-T1) 8192 cap → `SD_ERR_NO_MEM`.
  Stub answers are short so it never fires in tests; a verbose real-Claude answer could.
  Tracked under H6 (surfaces only with a real key) — raising the cap is a one-line change
  then, with real payload sizes to size against.
- **CI never executed on GitHub** — host + lint + esp-idf jobs are validated locally only;
  first PR will be the real test (esp-idf-build most likely to fail first — see H7).
- **`device.c text[1024]`** stack local is acceptable on 3584 B once T1 removes the 8 KB peak.
- **32-char SSID truncation (latent, hardware-path, untestable locally).** `net_esp.c:99`
  copies the SSID with `strncpy(..., sizeof(wifi_cfg.sta.ssid) - 1)` = 31 bytes, NUL-
  terminating at 31. `wifi_config_t.sta.ssid` is 32 bytes and an 802.11 SSID may use all
  32 (the field is not required to be NUL-terminated). A maximal 32-char SSID would lose
  its last character and fail to associate. Deliberately left unchanged: the `esp_wifi`
  semantics for a non-NUL-terminated 32-byte `ssid` field cannot be verified without the
  IDF headers/toolchain (not installed locally — see H7), and changing a Wi-Fi bring-up
  path blind risks a worse bug (esp_wifi reading past a 32-byte field with no NUL). Fix
  when H4 (Wi-Fi bring-up on silicon) is worked: copy `min(strlen, 32)` bytes and rely on
  the zero-initialized `wifi_cfg` for the terminator, validated against a 32-char SSID on
  real hardware.

---

## ✅ Done (verified, with commit)

### Red-team code review pass (trust nothing; re-derive from code)
- **T9** (P0) · malformed peer input crashed the **entire** phone server —
  `127bbc9`. `handle_connection` only treats `framing.ProtocolError`/`OSError`
  as "end this one connection"; any other exception escapes through `serve()`'s
  `accept()` loop and kills the process, dropping every future connection. Four
  host-reproducible leak paths were closed at the source (all now `ProtocolError`):
  `decode_control` on truncated/non-UTF-8/non-object/type-less JSON
  (`UnicodeDecodeError`/`JSONDecodeError`); `Image`/`AudioReassembler.__init__` on a
  missing or non-integer `begin` field (`KeyError`/`TypeError`, via new
  `_require`/`_require_int`); and `…Reassembler.finish()` on a correct chunk *count*
  but wrong `seq` numbers (`KeyError` in the `range()` join). Added 14 regression
  cases in `tests/test_framing.py`; 50 pytest + ruff green.
  *Why previously missed:* prior passes audited the C firmware memory-safety paths;
  the Python reference server's exception-boundary completeness was never tested —
  the happy-path image round-trip passed, masking the malformed-input gap.

### Independent verification pass (challenge "blocked", trust code only)
- **T8** (P0) · 32-bit integer-overflow → out-of-bounds write in
  `sd_msg_recv_audio_body` — *(commit below)*. The chunk-offset bounds check
  computed `(size_t)seq * SD_CHUNK_SIZE`; `seq` is peer-controlled and `size_t`
  is 32-bit on the ESP32, so `seq >= 2^20` wrapped the offset and defeated the
  check → OOB `memcpy` on hardware. The 64-bit host never reproduced it (no wrap),
  and the existing test used `seq=1000` (too small to wrap) — **false confidence**.
  Fixed with `uint64_t` offset math; added `test_audio_body_seq_offset_overflow`.
  *Why previously missed:* host is 64-bit; the bug is 32-bit-only and the test
  seq was below the wrap threshold.

### Maintainer pass (this round)
- **T1** (P0) · `sd_msg_recv` 8 KB stack buffer → heap — `f95da84`. Fixes the
  guaranteed ESP32 main-task stack overflow on the first control-frame read once
  `SUDONIT_RUN_UPLINK` runs on silicon. Behavior unchanged on host.
- **T2** (P0) · double-`close()` of the same fd in robustness tests — `ac5a07d`.
  Each fd now closed exactly once (EOF signaled via `sd_transport_close(w.b)`).
- **T4** (P1) · regression test for the large-control-frame path — `4448f26`.
  `test_recv_large_ai_response` drives a ~7 KB `ai_response` near the 8191-byte cap
  through the heap buffer; locks T1 against silent regression.
- **T5** (P2) · stale refs in `eval/run_eval.py` — `a423ddf`. Docstring/examples/
  `--dataset` help corrected (`tools/camera_degrade.py`, `benchmarks/_source`).
- **T6 + T7** (P3) · dangling doc refs — `109923b`. `CONTRIBUTING.md` MILESTONES.md →
  ROADMAP.md; `DEVELOPMENT.md` `ai_benchmark.py` → `eval/run_eval.py`.

### Earlier
- Pre-hardware code queue A–G: CI host job `9ff4228`, robustness tests `d2c74a4`,
  ESP-IDF CI `2551a1c`, uplink wiring `dd77d75`, latency instrumentation `2df7bf6`,
  Makefile `3c2aa69`, lint `192a876`.
- Simplification: dead-code/dup removal `7b466bf`, docs 71→22 `bd50924`, UX archive `69c1e4a`.
- README license fix, interop test, eval harness, hostile self-review of the branch.
