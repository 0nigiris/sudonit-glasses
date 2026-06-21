# DEVELOPMENT.md

> How to build, run, test, and contribute to Sudonit, plus the pre-hardware work plan.
>
> Consolidated document — built by merging the sources below verbatim under
> labeled dividers (no information removed). See DOCS_CONSOLIDATION_PLAN.md.


---

<!-- ===== consolidated from: RUNNING.md ===== -->

# RUNNING.md — running the Sudonit prototype

This is the hardware-free vertical slice of the V1 loop:

```
capture image -> transport -> AI provider -> text -> audio
```

It runs entirely on a development machine. No ESP32, no phone, no camera
required — the glasses are simulated and the "phone brain" runs locally. When
the hardware arrives, only the capture source changes; everything below it is
already proven here.

---

## Requirements

- Linux, Python 3.10+
- `espeak-ng` and `aplay` for audio (already present on most desktop installs)
- Python packages:
  - `pytest` — to run the tests
  - `anthropic` — **only** needed for the real Claude provider; the deterministic
    stub provider needs nothing beyond the standard library

```bash
python3 -m pip install anthropic pytest
```

The loop runs with **zero** third-party packages if you stay on the stub
provider (no API key) — `anthropic` is optional.

---

## Run the tests

No API key, no network needed (tests use the deterministic stub):

```bash
python3 -m pytest -q
```

Expected: `7 passed`.

---

## Run the full demo

Two terminals.

**Terminal A — the phone brain (data-plane server):**

```bash
python3 -m phone.server
```

It prints the selected AI provider and listens on `127.0.0.1:8765`.

**Terminal B — the firmware host loop (capture + send):**

Build the host firmware once, then run the real device loop against the server:

```bash
cmake -S firmware -B firmware/build && cmake --build firmware/build
./firmware/build/device_interop 127.0.0.1 8765
```

This runs the actual firmware `device.c` (mock camera capture) over the real
protocol — the same code the ESP32 build links against the hardware backends.

What you'll see: the glasses ping the phone, stream the image over the chunked
protocol, and receive the AI response back. The phone writes the spoken answer
to `run/<image-id>.wav` and plays it.

---

## Use real Claude vision instead of the stub

Set an API key before starting the phone server — no code change required:

```bash
export ANTHROPIC_API_KEY=sk-ant-...
python3 -m phone.server
```

The provider is auto-selected at startup (`phone/ai/__init__.py`): Claude if a
key is present and the SDK is installed, otherwise the stub.

Optional overrides:

| Env var                | Default            | Meaning                          |
|------------------------|--------------------|----------------------------------|
| `ANTHROPIC_API_KEY`    | _(unset)_          | Enables the real Claude provider |
| `SUDONIT_AI_PROVIDER`  | auto               | Force `anthropic` or `stub`      |
| `SUDONIT_AI_MODEL`     | `claude-opus-4-8`  | Vision model id                  |

---

## Generate a sample image manually

```bash
python3 tools/make_sample_image.py [out_path]
```

Pure-stdlib PNG writer (no Pillow) — a gradient with a red square and a yellow
bar, enough for a real vision model to describe.

---

## Layout

| Path          | What it is                                                    |
|---------------|---------------------------------------------------------------|
| `protocol/`   | Shared wire contract: framing + chunked image transfer. Both firmware and the Android app will implement this. See `protocol/TRANSPORT.md`. |
| `phone/`      | The phone brain: AI provider interface, pipeline, TCP server. |
| `simulator/`  | Hardware-free glasses stand-in.                               |
| `tools/`      | Helper scripts (sample image generator).                      |
| `tests/`      | Protocol + pipeline tests.                                    |
| `run/`        | Generated audio output (gitignored).                          |


---

<!-- ===== consolidated from: TEST_PLAN.md ===== -->

# TEST_PLAN.md — executable verification per subsystem

Focus: **things a machine can run and that pass or fail**, not review checklists. Three layers:

- **Unit/Host** — runs today on a dev machine, no hardware (`ctest`, `pytest`). Already partly
  in place (4/4 C, 16/16 Python). Expand here first; it is free and fast.
- **Hardware** — runs only on the board; defined now so day-one bring-up is measured, not
  vibed.
- **Success criteria** — the pass condition, tied to `DEMO_METRICS.md` where a number exists.

Current automated coverage exists for: protocol (`test_protocol.c`, `tests/test_framing.py`),
config (`test_config.c`), provisioning (`test_provisioning.c`), HAL contract (`test_hal.c`),
pipeline/image/server (`tests/test_*.py`). The gaps below are what to add.

---

## Protocol
- **Host (have):** framing round-trip, chunk reassembly, SHA-256 vectors, malformed-frame
  rejection. **Add:** fuzz the length prefix + truncated frames (bounded-read must return
  `SD_ERR_TIMEOUT`/`IO`, never hang or overrun).
- **Hardware:** ping/pong over real Wi-Fi (covered by app_main self-test).
- **Success:** all vectors pass host; on device a ping returns a pong over TCP.

## Camera
- **Host:** mock backend capture/release lifecycle (have, `test_hal`). **Add:** assert
  double-capture-without-release returns `SD_ERR_INVALID` (the `s_fb` guard in `camera_esp.c`).
- **Hardware:** `esp_camera_init` returns OK; one `esp_camera_fb_get` yields a JPEG; dump and
  open it; capture 20×, count intact (SHA on the phone side).
- **Success:** ≥ 19/20 intact JPEGs (DEMO_METRICS #3); first-light pin map confirmed.

## Audio Output
- **Host:** mock plays PCM, format-change re-init path (extend `test_hal`).
- **Hardware:** tone audible; speech intelligible; **buzz test** — repeat with camera streaming
  and assert no audible whine (FAILURE #8).
- **Success:** audio begins ≤ 1 s after text ready, no buzz with camera active (DEMO_METRICS #5).

## Microphone
- **Host:** stub returns `SD_ERR_UNSUPPORTED` (assert it stays a clean no-op).
- **Hardware:** deferred (not on V1 path).
- **Success:** N/A for V1 — explicitly out of scope.

## Wi-Fi
- **Host:** `net_host` + `transport_tcp` connect/timeout paths (have via interop). **Add:**
  connect-to-dead-host returns `SD_ERR_IO` within the connect timeout (no hang).
- **Hardware:** associate on a known 2.4 GHz AP; measure associate time ×10; one hostile-network
  attempt (captive/5 GHz) to confirm graceful failure.
- **Success:** associates ≤ 8 s, ≥ 9/10 (DEMO_METRICS #2); hostile network fails cleanly, no hang.

## BLE
- **All layers:** none — 0% built, out of V1 scope. No tests until it exists.

## Provisioning
- **Host (have):** `test_provisioning` REPL commands, `test_config` NVS/host store round-trip.
  **Add:** assert the password is never echoed by any REPL command (security regression guard).
- **Hardware:** provision over USB-serial REPL; reboot; confirm config persists in NVS.
- **Success:** config survives reboot; no secret ever printed.

## AI Pipeline
- **Host (have):** `test_pipeline`, `test_image` (invalid-image → speakable error),
  `test_server_e2e` (full protocol round trip with stub provider). **Add:** **T2** — device
  `main_loop` against a live `phone.server` over loopback asserting `ai_response` + PCM downlink.
- **Hardware:** the full loop (T11) returns a real Claude answer; log latency + correctness.
- **Success:** loop completes; round-trip median ≤ 2 s, correctness ≥ 8/10 (DEMO_METRICS #4/#9).

## Battery
- **Host:** stub path (assert `SD_ERR_UNSUPPORTED` doesn't crash the cycle once T8 makes it
  tolerant).
- **Hardware:** ADC read tracks a bench supply sweep (e.g. 3.3→4.2 V → plausible %); current
  draw logged idle/active/peak.
- **Success:** reported % monotonic with real voltage; peak current recorded (DEMO_METRICS #6/#7/#8).

## Display / UI Runtime / Storage(gallery)
- **All layers:** out of V1 scope — no tests planned. (Config storage is covered under
  Provisioning.)

---

## What to automate before June 29 (free, high value)

1. **T2 device-loop E2E** over loopback — locks the demo path against regression.
2. **Truncated/again-fuzzed framing** test — the protocol is the foundation; harden it.
3. **Password-never-logged** assertion — a security regression guard (CLAUDE.md mandate).
4. **Connect-to-dead-host timeout** test — proves the device won't hang on a missing phone.

> Principle: **every bug caught on the host is a bug not chased on a board during the one week
> that matters.** Host tests are cheap; bring-up time is not. Spend the pre-hardware window
> converting "should work" into "the test says it works."


---

<!-- ===== consolidated from: IMPLEMENTATION_GAP_REPORT.md ===== -->

# IMPLEMENTATION_GAP_REPORT.md — what code actually exists, per subsystem

Grounded in the source tree as of this report, not in the design docs. Two numbers are kept
**separate on purpose**:

- **Code %** — how much of the implementation is written and host-verified.
- **HW-validated %** — how much has run on real silicon. **This is 0% for every subsystem.**
  Nothing in this repository has ever executed on an ESP32. Keep that in view while reading
  any "75%" below — it means "75% written," never "75% working on hardware."

Evidence anchor (collected this session): the host build compiles clean under
`-Wall -Wextra -Werror`, **4/4 C tests pass** (`ctest`), **16/16 Python tests pass**
(`pytest`). That is real and it is the *only* thing currently proven.

Legend — **HW-dep:** does finishing it require the board? **Effort:** rough dev-days once
hardware is present.

---

## 1. Camera — Code ~75% · HW-validated 0%
- **Exists:** `src/hal/esp32/camera_esp.c` (173 lines) — a *real* OV5640 driver over the
  `esp32-camera` managed component (present in `managed_components/`): `esp_camera_init`,
  JPEG/SVGA in PSRAM, capture→release lifecycle. Wired into `device.c` (`sd_device_capture_cycle`,
  `sd_device_run_uplink`). Mock backend (`camera_mock.c`) drives host tests. Behind
  `SUDONIT_CAMERA_DRIVER`.
- **Missing:** verified pin map (the map at `camera_esp.c:34-49` is the *assumed* Freenove
  layout), confirmed PSRAM frame-buffer alloc, any real frame ever produced.
- **Blockers:** physical board; seller pinout diagram.
- **HW-dep:** Yes (total). **Effort:** 0.5–1 day if pins match; +1–2 if they don't.

## 2. Audio Output — Code ~75% · HW-validated 0%
- **Exists:** `src/hal/esp32/audio_esp.c` (133 lines) — real MAX98357A I2S-std TX driver
  (ESP-IDF v5.2 `i2s_std`), format re-init, PCM write. Downlink fully wired in
  `device.c::play_audio_downlink` (receives chunked PCM, SHA-checked, plays). Behind
  `SUDONIT_AUDIO_DRIVER`.
- **Missing:** verified I2S pins (`40/41/42` assumed), camera-active buzz check (FAILURE #8),
  any sound ever emitted.
- **Blockers:** board; speaker/amp wiring.
- **HW-dep:** Yes. **Effort:** 0.5–1 day.

## 3. Microphone — Code ~5% · HW-validated 0%
- **Exists:** `mic_esp.c` (23 lines) — **honest stub**, returns `SD_ERR_UNSUPPORTED`.
  HAL header `hal/mic.h` defines the contract.
- **Missing:** the entire I2S-RX driver.
- **Blockers:** board; **but also not on the V1 critical path** — the demo loop is
  image→AI→speech, no microphone required. This is the one stub safe to leave alone.
- **HW-dep:** Yes. **Effort:** 1–2 days — **deferrable past July 1.**

## 4. Wi-Fi — Code ~85% · HW-validated 0%
- **Exists:** `net_esp.c` (134 lines, STA bring-up, event-driven, 5-retry, 15 s timeout,
  password never logged) + `transport_wifi.c` (166 lines, bounded-wait TCP client via LwIP
  sockets). Self-test wired in `app_main.c` under `SUDONIT_NET_SELFTEST` (ping/pong). Host
  counterparts (`net_host.c`, `transport_tcp.c`) verified by tests.
- **Missing:** real association on real APs; robustness to hostile networks (FAILURE #11);
  reconnect policy beyond first session.
- **Blockers:** board; a known 2.4 GHz AP.
- **HW-dep:** Yes (validation). **Effort:** 0.5 day to first connect; field-hardening ongoing.

## 5. BLE — Code 0% · HW-validated 0%
- **Exists:** nothing. No source, no HAL header.
- **Missing:** everything (pairing, GATT, control plane).
- **Blockers:** none to start — but **not on the V1 critical path**; Wi-Fi is the data plane.
- **HW-dep:** No to prototype. **Effort:** 1–2 weeks — **do not start before July 1.**

## 6. Provisioning — Code ~70% · HW-validated 0%
- **Exists:** `config/config.c` (158) + `config_store_nvs.c` (72, on-target) +
  `config_store_host.c` (48) + `app/provisioning.c` (184, serial REPL command processor).
  Tested (`test_config`, `test_provisioning` both pass). Behind `SUDONIT_PROVISION_CONSOLE`.
- **Missing:** phone-assisted provisioning (the on-device REPL is fine over USB-serial for the
  demo; FAILURE #24 only bites for end-users, not bring-up).
- **Blockers:** none for demo.
- **HW-dep:** No (works on host). **Effort:** ~0 for demo; phone path is later.

## 7. AI Pipeline — Code ~80% · HW-validated 0%
- **Exists (phone side, all tested):** `phone/pipeline.py` (socket-free core loop),
  `phone/ai/anthropic_provider.py` (real Claude SDK, `claude-opus-4-8`, key from env),
  `stub_provider.py`, `phone/audio/tts.py`, `phone/image.py` (validation), `phone/server.py`
  (full protocol endpoint: reassemble image → pipeline → `ai_response` + PCM downlink).
  **Device side:** `device.c::sd_device_run_uplink` implements the full capture→send→receive→
  play turn.
- **Missing:** **`app_main` only runs ping/pong in the self-test — it does not yet call
  `sd_device_run_uplink`.** Wiring that is the single highest-value code change in the repo
  (see CODE_FIRST_BACKLOG T1). Plus: real-frame answer quality, real latency — unmeasured.
- **Blockers:** the wiring (no hardware needed to *write* it); an `ANTHROPIC_API_KEY` to run live.
- **HW-dep:** Partial — the loop can be exercised host↔host today. **Effort:** 0.5 day wiring.

## 8. Battery — Code ~5% · HW-validated 0%
- **Exists:** `battery_esp.c` (19 lines) — **honest stub**, `SD_ERR_UNSUPPORTED` (deliberately
  does not invent a percentage). HAL contract in `hal/battery.h`. Note: `device.c` currently
  *fails the capture cycle* if battery read fails — see CODE_FIRST_BACKLOG T8.
- **Missing:** ADC1 (GPIO1) VBAT-divider read + calibration.
- **Blockers:** board; divider resistor values.
- **HW-dep:** Yes. **Effort:** 0.5–1 day. Needed for DEMO_METRICS, not for the AI loop.

## 9. Display — Code 0% · HW-validated 0%
- **Exists:** nothing in firmware. No `hal/display.h`, no driver. Only design docs
  (`docs/DISPLAY_*`, `UI_*`).
- **Missing:** everything — **and per PRE-HARDWARE_STOP_LIST it should stay that way for V1.**
  V1 is audio-first; optics are unproven (ASSUMPTION C14, 15%).
- **HW-dep:** Yes. **Effort:** large — **explicitly out of scope before July 1.**

## 10. UI Runtime — Code 0% · HW-validated 0%
- **Exists:** nothing in firmware (logic validated only in the HTML simulator).
- **Missing:** all of it — **and gated on a display that may not exist.** Do not build.
- **HW-dep:** Yes. **Effort:** very large — **out of scope.**

## 11. Storage — Code ~40% (config only) · HW-validated 0%
- **Exists:** NVS-backed config store (see Provisioning). `partitions.csv` present.
- **Missing:** image/gallery storage — but the V1 loop streams frames to the phone and does
  **not** persist them, so gallery storage is **not on the critical path.**
- **HW-dep:** Partial. **Effort:** config done; gallery deferred.

## 12. Phone Companion — Code ~70% (desktop) / 0% (mobile) · HW-validated 0%
- **Exists:** the Python `phone/` server is a complete desktop "phone brain" and is what the
  glasses will TCP-connect to for the demo (`python -m phone.server`). Tested end-to-end
  (`tests/test_server_e2e.py`).
- **Missing:** an actual Android/iOS app (ASSUMPTION B12, 0% built). **For July 1, a laptop on
  the same Wi-Fi IS the phone brain** — the mobile app is a post-demo problem.
- **HW-dep:** No (laptop). **Effort:** ~0 for demo; mobile app = weeks, later.

## 13. Protocol — Code ~90% · HW-validated 0%
- **Exists:** `components/protocol/src/` — `framing.c` (55), `messages.c` (200), `json.c` (137),
  `sha256.c` (100). **Byte-identical sources compile on host and device.** Python mirror in
  `protocol/`. Heavily tested (`test_protocol` 285 lines + `tests/test_framing.py`).
- **Missing:** nothing critical for V1. The most finished, most de-risked subsystem.
- **HW-dep:** No. **Effort:** ~0.

---

## The shape of the gap

| Subsystem | Code % | On V1 critical path? | Verdict |
|-----------|-------:|----------------------|---------|
| Protocol | 90 | Yes | **Done.** Leave it. |
| Wi-Fi | 85 | Yes | Code ready; needs silicon. |
| AI Pipeline | 80 | Yes | Ready; **wire the loop into app_main (T1).** |
| Camera | 75 | Yes | Real driver; needs pins + silicon. |
| Audio Out | 75 | Yes | Real driver; needs pins + silicon. |
| Provisioning | 70 | Yes (demo) | Serial REPL fine for bring-up. |
| Phone (desktop) | 70 | Yes (laptop = brain) | Fine for demo. |
| Storage (config) | 40 | Config only | Enough. |
| Battery | 5 | Metrics only | Stub; ADC later. |
| Microphone | 5 | **No** | Leave stubbed. |
| BLE | 0 | **No** | Don't start. |
| Display | 0 | **No** | Out of scope. |
| UI Runtime | 0 | **No** | Out of scope. |

> **The critical-path code is ~75–90% written and 0% validated.** The entire July-1 problem
> is *not* "write more subsystems" — it is **verify pins, wire one loop, flash, and measure.**
> Everything below the line (mic/BLE/display/UI) is scope to *protect*, not build.


---

<!-- ===== consolidated from: PRE_HARDWARE_EXECUTION_PLAN.md ===== -->

# PRE_HARDWARE_EXECUTION_PLAN.md

> Concrete code/test/tooling/CI work that can be completed **before** the ESP32-S3
> hardware arrives on **June 29, 2026**. Grounded in the actual repository state (read,
> not assumed). No new architecture, vision, audits, website content, or roadmap docs —
> only things that move real code forward.
>
> Goal: when hardware lands, June 29 is a **flash-and-run** day, not a write-code day.

---

## Repository state this plan is built on (verified)

- **Host build (`firmware/CMakeLists.txt`)** — works: 4 binaries (`device_demo`,
  `device_interop`, `device_config`, `device_provision`) + 4 C tests via ctest
  (`hal`, `protocol`, `config`, `provisioning`).
- **Python side** — 8 pytest files incl. the cross-stack guard
  `tests/test_firmware_interop.py` (builds the host firmware, runs a real turn, asserts).
- **Device loop (`device.c`)** — `sd_device_run_uplink` is complete on host: capture →
  chunked image over transport → phone/AI → audio downlink played through the audio HAL.
- **ESP-IDF build (`firmware/esp-idf/`)** — compiles and boots. `app_main.c` reports HAL
  backends, inits NVS/config, exercises the app→HAL path, and has a Wi-Fi **ping/pong**
  data-plane self-test behind `SUDONIT_NET_SELFTEST`. It does **not** yet call
  `sd_device_run_uplink`.
- **ESP32 backends** — `net_esp.c` + `transport_wifi.c` implemented (unproven on
  silicon); `camera_esp.c` + `audio_esp.c` have **real drivers behind build flags**
  (`SUDONIT_CAMERA_DRIVER` / `SUDONIT_AUDIO_DRIVER`), honest stubs otherwise;
  `battery_esp.c` + `mic_esp.c` are pure stubs (`SD_ERR_UNSUPPORTED`).
- **Tooling/benchmarks** — `tools/` has `ai_benchmark.py`, `camera_degrade.py` (rich
  degradation presets), `end_to_end_demo.py`, `make_sample_image.py`; `benchmarks/` holds
  a 4-tier × 4-scene degraded-image dataset (16 images + sources).
- **CI** — only `claude.yml` and `claude-code-review.yml`. **No build/test CI exists.**
  This is the single biggest gap.

---

## Task table

Sorted by priority = (highest value × lowest risk × not blocked by hardware).
Hours are focused-effort estimates.

| # | Task | Hrs | Dependencies | Expected benefit | Blocked by HW? |
|---|------|-----|--------------|------------------|----------------|
| **1** | **CI: host build + test workflow.** GitHub Action: configure CMake, build all host targets, run `ctest`, run `pytest` (incl. the interop guard). | 3–4 | none | Locks the *entire* demo path against regression before silicon. Every future change is verified automatically. Cheapest possible insurance. | **No** |
| **2** | **Protocol robustness / property tests.** Host tests for framing/messages: truncated frames, oversized length prefix, partial chunks, wrong SHA-256, interleaved kinds. | 3–4 | none | Hardens the most demo-critical layer (the wire protocol). Bugs here break *every* turn; finds them with zero hardware. | **No** |
| **3** | **CI: ESP-IDF compile job.** Action using the `espressif/idf` Docker image: `idf.py build` of the firmware — both the default (stub) build and a `-D SUDONIT_*_DRIVER` flagged build. Compile only, no flashing. | 3–5 | Docker image; #1 ideally first | Catches firmware/driver compile breakage continuously. Guarantees the flagged-driver build still compiles so June 29 isn't a build-debug day. | **No** (compile, not run) |
| **4** | **Wire `app_main.c` → `sd_device_run_uplink` behind a flag** (e.g. `SUDONIT_FULL_LOOP`), alongside/replacing the ping-pong self-test. | 2–3 | #3 to keep it compiling; logically follows the existing self-test | The full capture→AI→audio loop becomes flash-and-run the instant the camera/audio driver flags are on. Removes day-one coding from the critical path. | **No** to write; validation is HW |
| **5** | **Degraded-image interop matrix test.** Drive the existing `benchmarks/` images (4 tiers) through the host pipeline / firmware loop; assert it survives every tier without crashing or mis-framing. | 3 | reuses #2 harness + `benchmarks/` | Proves the demo path is robust to *realistic* bad camera input (blur/low-light/noise) before a real camera exists. Turns the dataset into a regression guard. | **No** |
| **6** | **Claude evaluation harness + golden dataset.** Extend `ai_benchmark.py`: a golden expected-answer set for the 16 benchmark images and a scorer (does the answer name the key object?). Runs against `AnthropicProvider`. | 4–6 | needs `ANTHROPIC_API_KEY` to *run* (not to build); reuses `benchmarks/` | Repeatable, scored AI-quality measurement that slots real captured images in later. Builds the harness now; produces real numbers the moment a key is available. **Do not fabricate metrics.** | **No** (gated on API key, not HW) |
| **7** | **Latency / size instrumentation in the host loop.** Add timing + payload-size capture to a host run (capture→send→AI→audio) emitting a machine-readable line. | 2–3 | reuses `device_interop` | A baseline performance harness; the *same* tool ingests hardware numbers later, so HW latency is measured, not guessed. Targets the #2 risk (latency). | **No** (baseline); real numbers HW |
| **8** | **Build automation script / Makefile.** One command: configure+build host, run `ctest` + `pytest`, optional `idf.py build`. | 1–2 | none | Lowers contributor friction; the same script backs the CI in #1. Makes "clone → green tests" trivial (supports the README fix). | **No** |
| **9** | **Static analysis / lint in CI.** `clang-format`/`clang-tidy` for C, `ruff` for Python, wired into #1. | 2 | #1 | Cheap, continuous quality signal; catches whole classes of bugs without hardware. A green badge is also a trust signal for contributors. | **No** |
| 10 | **README license/status correctness fix.** Replace "License: To be determined" with Apache-2.0 (the `LICENSE` file already present) and "Pre-Prototype" status with the honest current state. *(Correctness fix, not new content; see `README_REWRITE_PLAN.md`.)* | 1 | none | Removes a glaring, credibility-killing contradiction at the repo front door before any public eyes arrive. | **No** |

**Total not-blocked, high-value work: ~24–33 focused hours** — more than enough to fill
the runway to June 29 with real progress.

---

## Tier summary

- **Do first (foundation):** #1, #2 — CI + protocol hardening. Everything else is safer
  once the test net exists.
- **Do next (firmware readiness):** #3, #4 — ESP-IDF compiles in CI, and the full loop is
  wired and waiting behind a flag.
- **Do if time (evidence + robustness):** #5, #6, #7 — degraded-input guard, scored AI
  eval harness, latency baseline.
- **Polish (low effort, high signal):** #8, #9, #10.

---

## The three answers

### 1. Single highest-value task to start immediately

**#1 — the CI host build + test workflow.** There is currently *no* automated build or
test in CI; the interop guard and all C/Python tests only run when someone remembers to.
Wiring them into GitHub Actions locks the **entire host demo path** (protocol → device
loop → phone → AI → audio) against regression *before* hardware can introduce noise. It is
~3–4 hours, near-zero risk, blocked by nothing, and it makes every subsequent task on
this list safe to do. Start here.

### 2. Last useful task completable before June 29

**#4 — wiring `app_main.c` to `sd_device_run_uplink` behind a flag.** This is the final
piece of *host-validatable* firmware code: once it's merged and compiling (verified by
#3), bringing up hardware becomes "enable the driver flags and flash," not "write the
main loop on day one." Finishing this before June 29 moves the full capture→AI→audio loop
off the critical path so day one is spent validating silicon, not authoring code. It's the
right note to end the pre-hardware runway on.

### 3. Impossible until hardware arrives

These cannot be truthfully done or validated without the ESP32-S3 + peripherals:

- **Validate `SUDONIT_CAMERA_DRIVER` on a real OV5640** — real capture, image quality,
  framebuffer timing.
- **Validate `SUDONIT_AUDIO_DRIVER` on a real MAX98357A** — actual I2S playback, audible
  output.
- **Implement + validate `battery_esp.c`** (ADC read of the VBAT divider) — needs the real
  board and divider.
- **Implement + validate `mic_esp.c`** (I2S read from the real microphone).
- **Prove Wi-Fi bring-up on silicon** — `net_esp.c` / `transport_wifi.c` are written but
  unproven on the chip.
- **Brownout / power-stability testing** — the #1 predicted failure; only measurable on the
  real power path under a capture+Wi-Fi burst.
- **Real latency, battery-life, and thermal numbers** — the harnesses (#6, #7) are built
  pre-hardware, but truthful numbers require the device. *Do not fabricate any of these.*
- **Real captured-image quality and the actual live demo turn** — the entire proof-of-life
  (`LAUNCH_READINESS.md`).
- **Flashing / partition table / NVS-on-device validation** — only verifiable on hardware.

Everything above the line in the task table is real progress that needs no silicon;
everything in this list is correctly deferred to June 29.


---

<!-- ===== consolidated from: PRE-HARDWARE_STOP_LIST.md ===== -->

# PRE-HARDWARE_STOP_LIST.md — work to STOP doing until hardware gives evidence

The project's failure pattern (WHY_LOSES #5) is precise: *effort went where it was pleasant,
not where it was risky.* Architecture, a UI runtime, eight apps, and a slick simulator — all
built on top of parts that were never ordered and assumptions that were never tested.

This list names the work that should **not** be done before hardware arrives, and *why each
item is currently a waste of effort.* The test for "waste" is simple:

> **If a real measurement could make this work wrong, throwing it away, then doing it before
> the measurement is speculative — and probably premature.**

Every item below builds on an **Unknown** from `TRUTH_TABLE.md`. Building on Unknowns is how
you accumulate beautiful work that reality later deletes.

---

## STOP 1 — New architecture documents
**Why it's waste now:** The architecture already reverses itself once (dumb-client →
autonomy-first), *adding* scope mid-"fastest-path-to-demo" on philosophy, not evidence
(WHY_LOSES warning signs). More architecture refines a structure resting entirely on Unknowns
(optics, power, latency, market). The doc-to-running-code ratio rising while nothing runs on
silicon is itself a named warning sign. **Any new architecture doc is decoration on an
unvalidated foundation.**

## STOP 2 — New features and applications
**Why it's waste now:** There are already 8 apps designed for a display that may not be
focusable (C14, 15% confidence) on a device whose core flow may lose to the phone (D21). A
ninth app, or any feature, multiplies work that V4/V16 could invalidate wholesale. WHY_WINS is
explicit: the winning version is *subtraction* — it **deleted** the gallery, settings registry,
and multi-app scope. Adding more is moving in the losing direction.

## STOP 3 — New UI systems / widgets / screens
**Why it's waste now:** The entire UI runtime assumes a readable near-eye display. If V4 shows
the optics can't focus (the most likely outcome), V1 ships **audio-first** and the UI runtime
is shelved (WHY_WINS cut it from V1). Polishing screens, widgets, layout, or the launcher
before the display is proven is building rooms onto a house with no foundation. Even the *logic*
is already validated in sim; more UI work adds no evidence, only surface area.

## STOP 4 — New abstractions, interfaces, HAL layers
**Why it's waste now:** Abstractions earn their keep by absorbing *known* variation. We don't
yet know the camera pin map (A2), whether audio buzzes (A7), the real latency profile (B10), or
whether there's a display at all (C14). An abstraction designed against guessed variation will
be wrong against measured variation and will have to be rebuilt. `sd_display`, the swappable
backends, the port seam — sufficient. **Build the second backend when you have the first real
one to generalize from**, not before.

## STOP 5 — Simulator expansion / polish
**Why it's waste now:** The simulator already over-promises — it shows color, emoji, instant
response, and readable text the 1-bit panel won't have (DESIGN_REVIEW; E24). Making it *prettier*
widens the gap between synthetic confidence and hardware reality, which is exactly how UX
confidence became "partly synthetic" in the postmortem. It has done its job (validated the state
machine). Further investment manufactures false certainty.

## STOP 6 — Protocol / format extensions for hypothetical needs
**Why it's waste now:** The wire protocol works host-side for the flows that exist. New frame
kinds, compression schemes, or sync formats for features that aren't validated (notifications,
calendar sync, nav overlays) optimize for demand that may never exist (D19/D20 unknown). Extend
the protocol when a *measured* need appears.

## STOP 7 — Performance / power *optimization* in code
**Why it's waste now:** CLAUDE.md already says optimization is secondary in the prototype phase.
You cannot optimize power without measuring it (A4/A5 are Unknown). Premature duty-cycling,
sleep-state tuning, or buffer-size fiddling guesses at numbers V6/V11 will hand you for free.
Measure first, then optimize the real bottleneck — not the imagined one.

## STOP 8 — Mechanical / industrial design beyond a test bracket
**Why it's waste now:** "Fits every frame" is an unbounded problem (WHY_LOSES) and the form
factor itself may be wrong (V2 public-wear test, D22). Detailed enclosure CAD, multi-frame
brackets, or aesthetic refinement before V2/V13 (social acceptability, thermal) is committing
to a shape before knowing if anyone will wear it or if it cooks the temple. **One rough test
bracket is enough to run the experiments.**

## STOP 9 — Companion-app feature work beyond bare setup + broker
**Why it's waste now:** The app is 0% built (B12) and its hardest fact — surviving OS
backgrounding (V15) — is untested. Designing app screens, settings sync, or a polished UI before
proving the broker even *stays alive* is decorating a function that may not be viable. Prove
persistence first.

---

## What is NOT on the stop list (do these instead)

The inverse of this list is the real to-do, and it's almost all **evidence-gathering that needs
little or no hardware** (`VALIDATION_BACKLOG.md` Tier 0–1):

- **Customer discovery** on BYO-key / niche (V1) — $0, attacks the killer question.
- **Wear a dummy in public** (V2) — $0, tests the form factor.
- **Phone-only latency spike** (V3) — half a day, tells you if the flow can ever win.
- **Order/bench cheap optics** (V4) — the one piece of *buying* worth doing now.
- **First-light bring-up scripts** ready to run the moment the board powers on (V5–V8) — these
  reduce *hardware risk*, which CLAUDE.md explicitly permits.

> Everything on the STOP list is pleasant, safe, and produces something to look at. Everything
> on the do-instead list is uncomfortable, risky, and produces *evidence*. The postmortem is the
> story of a team that kept choosing the first column. The whole point of going evidence-driven
> is to choose the second.

## The one rule that replaces this whole list

> **Do not build anything on top of an Unknown. Spend effort turning Unknowns into facts. Resume
> building only on what has moved to Known True — with a measurement attached.**
