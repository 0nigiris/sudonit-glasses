# HARDWARE.md

> Board resources, bring-up procedures, provisioning, and external references.
>
> Consolidated document — built by merging the sources below verbatim under
> labeled dividers (no information removed). See DOCS_CONSOLIDATION_PLAN.md.


---

<!-- ===== consolidated from: HARDWARE_ARRIVAL_CHECKLIST.md ===== -->

# HARDWARE_ARRIVAL_CHECKLIST.md — minute-by-minute first bring-up

**Goal:** shortest path from an unopened board to a **real Claude answer spoken by the device.**
Ordered so each step gates the next and a failure stops you before wasting time downstream.

**Prereqs (have these ready before the board arrives — see QUICK_WINS):**
- ESP-IDF v5.2 installed; `idf.py` on PATH.
- `tools/flash.sh` (QUICK_WINS Q5) wrapping the build flags.
- A laptop on a **2.4 GHz** Wi-Fi network running `python -m phone.server` with
  `ANTHROPIC_API_KEY` set. Note the laptop's LAN IP.
- An inline USB power meter.
- The **seller's pin diagram** for this exact board open on screen.

Each step: **command · expected log/result · failure signature · recovery.**

---

### T+0 min — Inspect & power (no firmware)
- **Do:** Visual check; plug in over USB; confirm enumeration.
- **Expect:** A serial port appears (`/dev/ttyACM0` or `ttyUSB0`).
- **Failure:** No port → bad cable (charge-only) or driver. **Recover:** known-good data cable;
  check CH34x/CP210x driver.

### T+5 min — Baseline boot (stubs only, no peripheral flags)
- **Cmd:** `idf.py -C firmware/esp-idf set-target esp32s3 && idf.py -C firmware/esp-idf build flash monitor`
- **Expect:** `Sudonit Smart Glasses firmware — ESP-IDF target boot`, then
  `HAL backends: camera=ov5640 (stub) battery=adc-vbat (stub) audio=max98357a (stub) ...`,
  then `boot complete — peripheral drivers are stubs pending hardware`. **PSRAM line in the IDF
  banner must show octal PSRAM detected.**
- **Failure:** boot loop / `Brownout detector` / no PSRAM. **Recover:** better USB supply;
  if no PSRAM, recheck `sdkconfig.defaults` octal flags + that it's truly an N16R8.
- **Evidence:** save the boot log → `evidence/01-boot.log` (FIRST_REAL_DATA_PLAN #1).

### T+15 min — Provision over serial REPL
- **Cmd:** `idf.py -C firmware/esp-idf build flash monitor -DSUDONIT_PROVISION_CONSOLE=1`
- **Do:** At the REPL set Wi-Fi SSID/password, `server_host` = laptop IP, `server_port` = 8765;
  save; confirm it reads back (password shown only as `(set)`).
- **Failure:** REPL absent → flag not applied. **Recover:** confirm the `-D` reached CMake
  (`idf.py fullclean` then rebuild).

### T+25 min — Wi-Fi + ping/pong on silicon (data plane)
- **Cmd:** `./tools/flash.sh net` (= `-DSUDONIT_NET_SELFTEST=1`)
- **Expect:** `got IP …`, then `net self-test: ping/pong OK — data plane is live`. Laptop server
  prints the connection.
- **Failure:** `Wi-Fi did not connect (timeout)` → 5 GHz-only AP or wrong creds. **Recover:**
  use a 2.4 GHz SSID / phone hotspot; re-provision. `connect … failed` → wrong IP/port or
  server not running / firewall. **Recover:** verify laptop IP, server listening, same subnet.
- **Evidence:** associate time → `evidence/08-wifi.csv`.

### T+40 min — Camera first light
- **Do:** **Verify every pin** in `camera_esp.c:34-49` against the seller diagram. Fix mismatches.
- **Cmd:** `./tools/flash.sh camera` (= `-DSUDONIT_CAMERA_DRIVER=1`)
- **Expect:** `camera sensor PID=0x… detected`; capture cycle logs `captured 800x600 image/jpeg
  (N bytes)`. Dump the JPEG and open it.
- **Failure:** `esp_camera_init failed (0x…)` → wrong pins / no PSRAM / bad sensor. **Recover:**
  try the matching esp32-camera board preset; reseat the ribbon; confirm PSRAM from T+5.
  Garbage/black frame → pin map close but wrong (often a data-bit swap). **Recover:** compare
  bit-by-bit to the diagram.
- **Evidence:** the JPEG → `evidence/03-frame.jpg`; 10 scenes → `evidence/04-scenes/`.

### T+60 min — Audio first sound + buzz check
- **Do:** Confirm I2S pins `40/41/42` (`audio_esp.c:34-36`) are broken out and don't clash with
  the now-verified camera pins.
- **Cmd:** `./tools/flash.sh audio` (= `-DSUDONIT_AUDIO_DRIVER=1`)
- **Expect:** `I2S TX up`; a tone is audible; speech is intelligible. Repeat **with the camera
  streaming** — listen for whine.
- **Failure:** silence → wrong/again-strapped pins, amp SD pin low. **Recover:** fallback GPIOs;
  check amp wiring/gain straps. Buzz with camera → PCLK coupling (FAILURE #8). **Recover:**
  separate the wires; note as a known issue, don't block the demo.
- **Evidence:** `evidence/05-audio.wav`.

### T+75 min — THE FULL LOOP (the demo) ⭐
- **Cmd:** `./tools/flash.sh loop` (= camera + audio + net + the T1 `SUDONIT_AI_LOOP`)
- **Expect:** capture → `uplink: captured …` → laptop server logs the image + Claude latency →
  device logs `ai_response: …` → `played N audio frames` → **you hear the answer.**
- **Failure:** loop stalls after capture → server not replying (check the key / provider error
  on the laptop). No audio but text OK → audio path (revisit T+60). Reset mid-loop → **brownout**
  (next step).
- **Evidence:** per-hop latency → `evidence/06-latency.csv`; answers → `evidence/07-correctness.csv`.

### T+90 min — Brownout / current pass
- **Do:** Re-run the loop with the **inline USB meter**; record peak mA and any resets.
- **Failure:** resets on camera+Wi-Fi spike → the #1 predicted failure. **Recover:** bulk cap on
  the supply; sequence init (bring Wi-Fi up, settle, then camera); lower JPEG/frame size.
- **Evidence:** `evidence/02-current.csv`.

---

## The one-screen day-one flow

```
inspect → boot(stub) → provision → wifi/ping → camera → audio → FULL LOOP → current
  T+0       T+5          T+15        T+25       T+40     T+60      T+75       T+90
   │         │            │           │          │        │         │          │
 port?    PSRAM?       config?     IP+pong?   JPEG?    sound?   ANSWER?     no reset?
```

**Target: a spoken Claude answer within ~90 minutes of opening the box**, with every step's
evidence written to `evidence/`. If camera pins are wrong, T+40 absorbs the slip; everything
else is flashing and listening. Do **not** detour into mic/BLE/display — they are not on this
path (PRE-HARDWARE_STOP_LIST).


---

<!-- ===== consolidated from: docs/HARDWARE_INTEGRATION_PLAN.md ===== -->

# HARDWARE_INTEGRATION_PLAN.md

What happens, step by step, when the ESP32-S3 and its peripherals arrive.

Status: planning (pre-hardware)
Target hardware (REFERENCES.md): ESP32-S3 N16R8 · OV5640 camera · ICS43434 /
INMP441 I2S microphone · MAX98357A I2S amplifier + open-ear speaker ·
TP4056 USB-C charger · 502080 1000mAh battery.

---

## Guiding principle

By the time hardware arrives, every subsystem above the hardware line should
already exist, be tested, and be documented (see PRE_HARDWARE_ROADMAP.md). The
remaining work is to implement each real driver **behind an interface that
already exists** — the Hardware Abstraction Layer (HAL). Integration is then
"swap a mock for a real driver," one peripheral at a time, not "build the
system."

The HAL seams (defined pre-hardware):

| HAL interface          | Mock (pre-hardware)        | Real driver (on arrival)        |
|------------------------|----------------------------|---------------------------------|
| `hal/camera.h`         | baked-in JPEG/PNG bytes     | OV5640 via esp32-camera         |
| `hal/audio.h`          | write PCM/WAV to file/log   | MAX98357A over I2S              |
| `hal/mic.h`            | replay a recorded sample    | ICS43434/INMP441 over I2S       |
| `hal/battery.h`        | fixed/scripted percentage   | ADC read of divider on VBAT     |
| `hal/transport.h`      | TCP socket (host)           | Wi-Fi data plane (**built**) + BLE control |
| `hal/net.h`            | no-op (OS owns the network) | Wi-Fi STA bring-up (**built**)  |

---

## 1. ESP32-S3 board bring-up

1. Install/confirm ESP-IDF v5.2 (already present) and the `esp32s3` target.
2. Add the ESP-IDF build alongside the existing host build (same `firmware/`
   tree, second `CMakeLists` target — the HAL and app code are shared).
3. Flash a minimal "hello" firmware; confirm USB-serial (`tio`/`screen`) and the
   `SD_LOG` output appears.
4. Confirm PSRAM (N16R8 = 8 MB) is detected and usable — image buffers depend on
   it.
5. Bring up Wi-Fi STA: associate with a test AP, open a TCP connection to the
   running `python -m phone.server`, send a `ping`, expect `pong`. **This proves
   the transport HAL on real silicon with zero peripherals attached.**

   *Firmware side already built:* `net_esp.c` (`sd_net_start`) associates with the
   provisioned SSID/password and waits for an IP; `transport_wifi.c` is the LwIP
   TCP client implementing `hal/transport.h` (the same all-or-nothing client as
   the host build). A guarded self-test in `app_main` runs exactly this ping/pong
   — provision creds with `device_provision`, then build with
   `idf.py build -DSUDONIT_NET_SELFTEST=1` and watch the serial log. What remains
   is running it on real silicon against a live AP (no hardware yet).

Exit criteria: board boots project firmware, logs over serial, and completes a
ping/pong with the phone-brain over Wi-Fi.

## 2. Camera (OV5640) integration

1. Wire OV5640 to the DVP pins per the pin-map (to be finalized in
   `hardware/pinmap.md`); double-check the 18-pin DVP block doesn't collide with
   the I2S pins reserved for audio.
2. Add the `esp32-camera` component; configure for OV5640, JPEG output, a frame
   size that keeps a single frame comfortably inside PSRAM.
3. Implement `hal/camera.h` → `camera_esp.c`: `camera_capture()` returns the JPEG
   bytes + length + media type (`image/jpeg`).
4. Drop-in replace the mock camera in the device loop. **No other code changes.**
5. Capture one frame, stream it via the (already-tested) protocol to the
   phone-brain, confirm the AI describes the real scene.

Exit criteria: a real OV5640 frame reaches the phone and produces an AI
response — this is Phase 1's success criterion.

## 3. Audio output (MAX98357A) integration

1. Wire MAX98357A to the I2S-out pins; confirm I2S-out and the camera DVP block
   share no pins.
2. Implement `hal/audio.h` → `audio_esp.c`: `audio_play_pcm()` writing the I2S
   TX channel.
3. Decide the audio path: phone sends rendered PCM/Opus over the data plane
   (preferred — keeps the glasses dumb), the glasses just play it.
4. Play a known test tone, then a `play_audio` payload from the phone.

Exit criteria: a `play_audio` message from the phone is audible on the speaker.

## 4. Microphone (ICS43434 / INMP441) integration

1. Wire the I2S microphone (separate I2S peripheral or shared bus with care).
2. Implement `hal/mic.h` → `mic_esp.c`: `mic_read()` filling a PCM buffer.
3. Capture a few seconds of PCM, stream to the phone over the data plane (new
   `audio_begin`/`audio_end` types — document in PROTOCOL.md first), confirm
   round-trip.

Exit criteria: spoken audio captured on the glasses arrives intact at the phone.

## 5. Battery + charging (TP4056 / divider) integration

1. Wire the VBAT voltage divider to an ADC-capable pin; wire TP4056 USB-C.
2. Implement `hal/battery.h` → `battery_esp.c`: ADC read → calibrated percentage.
3. Emit `battery_status` over the control plane on a timer.
4. Verify charge/discharge readings track a multimeter within tolerance.

Exit criteria: real battery percentage reported to the phone; charging detected.

---

## Testing sequence (strict order — do not skip)

1. **Board boots + serial logs** (no peripherals).
2. **Wi-Fi + transport** ping/pong to phone (no peripherals).
3. **Camera** capture → phone → AI response.
4. **Audio out** play_audio audible.
5. **Microphone** capture → phone.
6. **Battery** reporting + charging.
7. **Concurrent** camera + Wi-Fi + audio under load (the realistic stress case).
8. **Power draw** measured with USB meter against POWER_BUDGET (to be written).

Each step is independently checkable because each sits behind one HAL interface;
a failure localizes to one driver, not the whole system.

---

## Validation checklist

- [ ] Board enumerates over USB; serial logging works.
- [ ] PSRAM detected; single JPEG frame fits with headroom.
- [ ] Wi-Fi associates; TCP ping/pong with phone succeeds.
- [ ] `camera_capture()` returns a valid JPEG; size within budget.
- [ ] Image SHA-256 verified on the phone after transfer (already enforced).
- [ ] AI response returns for a real captured scene.
- [ ] `audio_play_pcm()` produces clean audio; no I2S underruns.
- [ ] `mic_read()` captures intelligible PCM.
- [ ] Battery percentage within ±5% of a multimeter reading.
- [ ] Charging state detected.
- [ ] Camera + Wi-Fi + audio run concurrently without crash/brownout.
- [ ] Idle and active current draw recorded.
- [ ] Pin map matches the wired board exactly.

---

## Expected risks

| Risk | Severity | Note |
|------|----------|------|
| DVP camera pins collide with I2S audio pins | High | Resolve in pin-map **before** ordering/wiring; budget pins for camera first. |
| Brownout under camera + Wi-Fi + audio concurrent load | High | Battery is priority #2 and unmodeled; measure early, add capacitance/regulation. |
| BLE/Wi-Fi coexistence instability | Medium | Both radios share the 2.4 GHz front end; test the control+data split under load. |
| PSRAM bandwidth / frame buffering stalls | Medium | Keep frame size modest; one frame at a time. |
| Thermal near the temple | Medium | Camera + radio against skin; monitor during sustained use. |
| I2S clock conflicts (mic + amp) | Medium | Use separate I2S peripherals or a verified shared clock plan. |
| OV5640 sourcing/variant differences | Low | Keep OV2640 as a fallback (REFERENCES.md). |

---

## Fallback plans

- **Camera transport too slow / unstable over BLE:** already mitigated — data
  plane is Wi-Fi by decision (DECISIONS.md). If Wi-Fi association is flaky,
  fall back to a wired/USB bring-up for the camera path while debugging.
- **OV5640 unavailable or problematic:** fall back to OV2640 (lower res, well-
  supported); the HAL `camera_capture()` contract is unchanged.
- **MAX98357A / open-ear audio underwhelms:** the audio HAL is codec-agnostic;
  swap the amp or move to a different speaker without touching app code.
- **Battery placement/weight problems:** dual-arm layout (REFERENCES.md); the
  battery HAL only reads a percentage, unaffected by physical layout.
- **Concurrent-load brownout:** reduce capture frequency, lower Wi-Fi TX power,
  stagger camera vs. radio activity; worst case, capture-then-transmit instead
  of capture-while-transmitting.
- **Pin exhaustion:** drop the least-critical peripheral for the first wearable
  (e.g. defer the mic) — the HAL lets a peripheral be absent without breaking
  the build (mock stays in place).


---

<!-- ===== consolidated from: docs/BOARD_RESOURCES.md ===== -->

# BOARD_RESOURCES.md

**Single source of truth** for hardware resource allocation on the ordered board:
**ESP32-S3-WROOM-1 N16R8 CAM + OV5640** (AliExpress "A+A+A Store" class).

Purpose: when the board arrives, start *testing*, not researching. Everything
that must be verified on day one is marked **[VERIFY]**. The pin numbers here are
the authoritative copy of the `#define`s in `camera_esp.c` / `audio_esp.c`; if you
change a pin in code, change it here too.

Detailed per-subsystem plans: `CAMERA_BRINGUP.md`, `AUDIO_BRINGUP.md`.

---

## 1. Master GPIO allocation (ESP32-S3: GPIO 0–21, 26–48; 22–25 do not exist)

| GPIO | Allocated to | Status |
|---|---|---|
| 0  | Strapping — boot mode (internal pull-up) | RESERVED |
| 1  | **Battery ADC** (ADC1_CH0) — planned | FREE → reserve |
| 2  | spare (ADC1_CH1) | FREE |
| 3  | Strapping — JTAG select (ADC1_CH2) | RESERVED (avoid) |
| 4  | Camera SIOD / SCCB SDA | CAMERA **[VERIFY]** |
| 5  | Camera SIOC / SCCB SCL | CAMERA **[VERIFY]** |
| 6  | Camera VSYNC | CAMERA **[VERIFY]** |
| 7  | Camera HREF | CAMERA **[VERIFY]** |
| 8  | Camera D2 (Y4) | CAMERA **[VERIFY]** |
| 9  | Camera D1 (Y3) | CAMERA **[VERIFY]** |
| 10 | Camera D3 (Y5) | CAMERA **[VERIFY]** |
| 11 | Camera D0 (Y2) | CAMERA **[VERIFY]** |
| 12 | Camera D4 (Y6) | CAMERA **[VERIFY]** |
| 13 | Camera PCLK | CAMERA **[VERIFY]** |
| 14 | spare (I2S fallback) | FREE |
| 15 | Camera XCLK | CAMERA **[VERIFY]** |
| 16 | Camera D7 (Y9) | CAMERA **[VERIFY]** |
| 17 | Camera D6 (Y8) | CAMERA **[VERIFY]** |
| 18 | Camera D5 (Y7) | CAMERA **[VERIFY]** |
| 19 | Native USB D− | RESERVED |
| 20 | Native USB D+ | RESERVED |
| 21 | spare (I2S fallback) | FREE |
| 26–32 | In-package SPI flash | RESERVED (not exposed) |
| 33–37 | **Octal PSRAM** (the "R8") | RESERVED (not exposed) |
| 38 | spare | FREE |
| 39 | spare | FREE |
| 40 | **I2S DIN** (audio) | AUDIO **[VERIFY exposed]** |
| 41 | **I2S WS/LRC** (audio) | AUDIO **[VERIFY exposed]** |
| 42 | **I2S BCLK** (audio) | AUDIO **[VERIFY exposed]** |
| 43 | UART0 TXD0 (boot/console log) | RESERVED |
| 44 | UART0 RXD0 (console) | RESERVED |
| 45 | Strapping — VDD_SPI voltage | RESERVED |
| 46 | Strapping — boot/ROM | RESERVED |
| 47 | spare | FREE |
| 48 | spare (RGB LED on many devkits) | FREE **[VERIFY]** |

No logical collision exists: camera occupies GPIO ≤18, audio 40–42, battery 1.
The only open questions are **physical breakout** of the FREE/audio pins and
**confirming the camera map** — both resolved by the checklist in §8.

---

## 2. Camera pins  (driver: `camera_esp.c`, plan: `CAMERA_BRINGUP.md`)

Assumed **Freenove ESP32-S3-CAM** pinout (most generic N16R8 S3-CAM clones copy
it). 15 signals: XCLK 15, SIOD 4, SIOC 5, VSYNC 6, HREF 7, PCLK 13, D0 11, D1 9,
D2 8, D3 10, D4 12, D5 18, D6 17, D7 16; PWDN/RESET = −1 (unused).

**[VERIFY]** against the seller's pin diagram before first flash. If different, it
almost certainly matches another `esp32-camera` preset (`camera_pins.h`) — copy
that block into the `CAM_PIN_*` defines. Wrong pins → `esp_camera_init` fails or
garbage frames.

---

## 3. I2S audio pins  (driver: `audio_esp.c`, plan: `AUDIO_BRINGUP.md`)

MAX98357A: **BCLK 42, WS/LRC 41, DIN 40** (no MCLK, no data-in). Chosen to avoid
every committed pin. **[VERIFY]** these three are broken out on the board's
headers; fallbacks if not: 47, 48, 21, 14. The amp GAIN/SD pins are strapped in
hardware, not driven by firmware.

---

## 4. Battery ADC pins

- Must use **ADC1** (GPIO1–10): ADC2 is unusable while Wi-Fi is active.
- Camera consumes GPIO4–10, so the free ADC1 pins are **GPIO1, 2** (GPIO3 is a
  strapping pin — avoid). **Plan: VBAT divider → GPIO1.**
- Not yet implemented (`battery_esp.c` is a stub; battery is product priority #2,
  off the first-demo critical path). Pin is reserved here so audio/buttons don't
  claim it. **[VERIFY]** GPIO1 is exposed.

---

## 5. UART / console pins

- **UART0 = GPIO43 (TX) / GPIO44 (RX)** — carries the boot ROM log and the
  `SD_LOG`/console output. Reserved; do not reuse.
- The board also has **native USB-Serial-JTAG (GPIO19/20)** for flashing and
  monitoring over USB-C — no external USB-UART adapter needed.
- **[VERIFY]** which one is the active console in the built firmware (`idf.py
  monitor` will show output on whichever is configured); keep both pin pairs free
  of other peripherals.

---

## 6. Strapping pins (do not repurpose without care)

ESP32-S3 strapping pins: **GPIO0, GPIO3, GPIO45, GPIO46**.
- **GPIO0** — boot mode (pull-up = run, pull-down = download). Internal pull-up.
- **GPIO3** — JTAG source select; also ADC1_CH2. Avoid using as a normal IO.
- **GPIO45** — VDD_SPI voltage select (1.8 V vs 3.3 V). Leave as the board sets it.
- **GPIO46** — ROM boot/download control; sampled at reset.
None of these are used by camera/audio/battery here. **[VERIFY]** nothing on the
board ties them in a way that blocks normal boot (it shouldn't — they are factory
strapped).

---

## 7. PSRAM & flash assumptions

| Assumption | Basis | Verify on day one |
|---|---|---|
| 8 MB **octal (OPI)** PSRAM | "R8" in N16R8 | Boot log prints ~8 MB PSRAM **[VERIFY]** |
| PSRAM mode = octal | `CONFIG_SPIRAM_MODE_OCT` already set | Camera init succeeds (needs PSRAM) |
| 16 MB flash | "N16" in N16R8 | `CONFIG_ESPTOOLPY_FLASHSIZE_16MB` already set |
| Frame buffer fits | SVGA JPEG ≪ 8 MB | First capture returns non-zero JPEG |

The board matching the existing `sdkconfig.defaults` exactly is the single
biggest pre-paid win — **no PSRAM/flash config change is needed**. The classic
"PSRAM not detected because the mode is wrong" failure should not occur; confirm
via the boot log anyway.

---

## 8. Remaining hardware uncertainties (the full list)

Everything still unverified, in priority order:

1. **Camera pin map** — assumed Freenove layout; could differ per listing. *(§2)*
2. **Audio pin breakout** — 40/41/42 must be physically exposed. *(§3)*
3. **PSRAM detection** — should match sdkconfig, but unconfirmed on silicon. *(§7)*
4. **Console routing** — UART0 vs USB-Serial-JTAG as the active log. *(§5)*
5. **OV5640 vs OV2640 detect** — sensor PID at init (driver handles both). *(§2)*
6. **Power/brownout** — camera + Wi-Fi (+ audio) current draw unmeasured;
   power from USB for the first demo, measure with a USB meter. *(priority risk)*
7. **Battery ADC pin** — GPIO1 exposure; divider ratio (future). *(§4)*
8. **Net stack on silicon** — `net_esp`/`transport_wifi` compiled, never
   associated with a real AP. *(separate from pins)*

Items 1–5 are answered in minutes with the board in hand; #6 needs a meter; #7–8
are off the first-demo critical path.

---

## 9. Day-one bring-up checklist (single consolidated list)

Do in order — each step gates the next, and each failure localizes to one thing.

1. **Flash the default firmware** (no feature flags). Confirm: board enumerates
   over USB-C, `idf.py monitor` shows the boot banner and HAL-backend log. *(proves
   toolchain + board + console before any peripheral variable)*
2. **Confirm PSRAM** in the boot log (~8 MB). *(§7)*
3. **Confirm the camera pin map** against the seller diagram; fix `CAM_PIN_*` if
   needed. *(§2 — highest-risk step)*
4. **Wi-Fi self-test:** provision creds (serial console build), then
   `-DSUDONIT_NET_SELFTEST=1` → expect Wi-Fi associate + TCP ping/pong with
   `python -m phone.server`. *(first on-silicon test of net_esp/transport_wifi)*
5. **Camera up:** `-DSUDONIT_CAMERA_DRIVER=1` → expect `camera sensor PID=...` and
   a non-zero SVGA JPEG from one capture. *(§2/§4 of CAMERA_BRINGUP)*
6. **Camera → phone → AI:** run the uplink → SHA-256 passes, AI response returns
   (stub first, then `ANTHROPIC_API_KEY`). *(the camera→AI success criterion)*
7. **Audio out:** verify GPIO 40/41/42 are exposed, wire the MAX98357A,
   `-DSUDONIT_AUDIO_DRIVER=1` → spoken answer comes out of the speaker. *(AI→audio)*
8. **Full demo:** camera + audio + net together (`-DSUDONIT_CAMERA_DRIVER=1
   -DSUDONIT_AUDIO_DRIVER=1`) → capture → ESP32 → phone → AI → speaker. **← goal.**
9. **Measure** idle/active current with a USB meter against the brownout risk;
   only then raise camera resolution / tune audio.

Everything above the HAL (protocol, chunked transfer, pipeline, audio downlink,
timeouts) is already proven on host, so a green run here is gated only on pins,
PSRAM, and the radio — not on the software stack.


---

<!-- ===== consolidated from: docs/CAMERA_BRINGUP.md ===== -->

# CAMERA_BRINGUP.md

Day-one plan for the camera on the **exact ordered board**:
**ESP32-S3 N16R8 CAM + OV5640** (AliExpress "A+A+A Store" class).

Status: driver written (`firmware/src/hal/esp32/camera_esp.c`), compiles and links
against `espressif/esp32-camera` 2.1.7, **never run on silicon**. Everything below
the pin map is the firmware-side preparation; the rest is verification work for
the moment the board arrives.

Related: `HARDWARE_INTEGRATION_PLAN.md` §2 (camera), `camera_esp.c` (the driver).

---

## 1. Board facts that matter for the camera

| Property | Value | Consequence |
|---|---|---|
| Module | ESP32-S3-WROOM-1 **N16R8** | 16 MB flash + **8 MB octal (OPI) PSRAM** |
| PSRAM mode | **Octal / OPI** | `sdkconfig.defaults` already sets `CONFIG_SPIRAM_MODE_OCT` — **no change needed** |
| Flash size | 16 MB | `CONFIG_ESPTOOLPY_FLASHSIZE_16MB` already set — **no change needed** |
| Sensor | **OV5640** (5 MP, autofocus-capable) | esp32-camera has an OV5640 driver + AF; we use plain JPEG capture |
| Camera bus | 8-bit DVP + SCCB | 15 pins consumed — confirm they don't collide with the I2S audio pins |

The single biggest pre-paid win: **N16R8 matches the existing sdkconfig exactly**,
so the classic "PSRAM not detected because the mode is wrong" failure should not
occur on this board. Verify anyway (checklist step 3).

---

## 2. Pin map — ⚠️ ASSUMPTION, verify before first flash

`camera_esp.c` uses the **Freenove ESP32-S3-CAM** pinout, which most generic
N16R8 S3-CAM clones copy. It is **not confirmed** for this specific listing.

| Signal | GPIO | Signal | GPIO |
|---|---|---|---|
| XCLK | 15 | VSYNC | 6 |
| SCCB SDA (SIOD) | 4 | HREF | 7 |
| SCCB SCL (SIOC) | 5 | PCLK | 13 |
| D7 (Y9) | 16 | D3 (Y5) | 10 |
| D6 (Y8) | 17 | D2 (Y4) | 8 |
| D5 (Y7) | 18 | D1 (Y3) | 9 |
| D4 (Y6) | 12 | D0 (Y2) | 11 |
| PWDN | -1 (unused) | RESET | -1 (unused) |

**Day-one task #1: confirm every pin against the seller's pin diagram / silk.**
If it differs, it almost certainly matches another esp32-camera preset — compare
against `managed_components/espressif__esp32-camera/.../camera_pins.h`
(`CAMERA_MODEL_ESP32S3_EYE`, `…_FREENOVE_ESP32S3_CAM`, etc.) and copy the matching
block into the `CAM_PIN_*` defines. Wrong pins = `esp_camera_init` fails or
garbage frames.

---

## 3. PSRAM & memory requirements

- **PSRAM is mandatory.** The JPEG framebuffer is allocated in PSRAM
  (`fb_location = CAMERA_FB_IN_PSRAM`). If PSRAM is not initialised/detected,
  `esp_camera_init` fails. N16R8 octal PSRAM + the existing octal sdkconfig should
  Just Work — confirm the boot log shows the PSRAM size.
- **Frame size (first bring-up): `FRAMESIZE_SVGA` (800×600), JPEG, `fb_count = 1`,
  `jpeg_quality = 12`.** Deliberately modest: a single SVGA JPEG is ~20–60 KB,
  trivial for 8 MB PSRAM, and keeps the pixel clock inside DMA bandwidth. Raise
  resolution only after the path is proven (OV5640 can do up to QSXGA, but big
  frames stress PSRAM bandwidth, Wi-Fi airtime, and the AI latency budget — and
  the phone vision model does not need 5 MP).
- **Internal RAM:** the DVP DMA line buffers live in internal RAM; SVGA/`fb_count=1`
  keeps this small. If `fb_count` is raised later, watch internal DRAM headroom.
- **Transfer:** a JPEG frame rides the already-proven chunked image transfer
  (`image_begin` → 4 KB chunks → `image_end`, SHA-256 verified). No protocol
  change is needed when the camera replaces the mock.

---

## 4. Likely bring-up failure modes

| Symptom | Most likely cause | Fix |
|---|---|---|
| `esp_camera_init failed (0x...)` | Wrong pin map | Verify pins vs seller diagram (§2) |
| `esp_camera_init` fails, PSRAM 0 in boot log | PSRAM not detected / wrong mode | Confirm octal PSRAM + `CONFIG_SPIRAM_MODE_OCT` |
| Init OK but sensor PID unexpected | Sensor isn't OV5640 / SCCB address | Check PID log; confirm SDA/SCL pins |
| `esp_camera_fb_get` returns NULL | XCLK wrong / DVP signal integrity | Check XCLK pin/freq; reseat FPC; lower `xclk_freq_hz` |
| Garbage / torn / half frames | PCLK/VSYNC/HREF mis-wired or noisy | Verify those 3 pins; shorten/clean wiring |
| Frames OK but pink/green tint | Sensor reg defaults / clock | Acceptable for bring-up; tune later |
| Brownout / reset on capture | Camera + Wi-Fi current spike | Power from USB; add bulk capacitance; measure with USB meter |
| Phone rejects image | Malformed JPEG | The phone validator (`phone/image.py`) already catches this with a clear error |
| DVP pin clashes with I2S audio | Shared GPIO | Resolve pin map **before** wiring audio (HARDWARE_INTEGRATION_PLAN risk #1) |

---

## 5. Day-one bring-up checklist (ordered — do not skip)

1. **Confirm the pin map** against the seller's diagram; correct `CAM_PIN_*` if
   needed (§2). This is the highest-risk step.
2. **Flash the plain firmware first** (no camera flag): confirm the board boots,
   USB-serial logging works, and the boot banner prints — proves the toolchain
   and the board before adding the camera variable.
3. **Confirm PSRAM** in the boot log (size ≈ 8 MB). The camera depends on it.
4. **Build + flash with the camera driver:**
   `idf.py build -DSUDONIT_CAMERA_DRIVER=1 flash monitor`.
5. **Watch for `camera sensor PID=0x... detected`** and a successful init. If
   init fails, go back to §2/§4.
6. **Capture one frame** (wire a call to `sd_camera_capture` / run the uplink) and
   confirm a non-zero JPEG length and sane width/height (800×600).
7. **Send it to the phone:** `python -m phone.server` + the uplink; confirm
   SHA-256 passes and an AI response comes back (this is the **camera→AI success
   criterion**). With the stub provider first, then with `ANTHROPIC_API_KEY`.
8. **Only then** raise resolution / tune quality, and measure current draw with a
   USB meter against the brownout risk.

Build flags reference: `-DSUDONIT_CAMERA_DRIVER=1` (real OV5640 driver),
`-DSUDONIT_NET_SELFTEST=1` (Wi-Fi ping/pong), `-DSUDONIT_PROVISION_CONSOLE=1`
(serial provisioning). All off by default.

---

## 6. What is already done (firmware side)

- Real OV5640 driver implementing the `camera.h` contract (JPEG → `sd_image_t`,
  zero-copy framebuffer with proper release), behind `SUDONIT_CAMERA_DRIVER`.
- `espressif/esp32-camera` 2.1.7 wired as a managed component
  (`main/idf_component.yml`); both builds verified — default (stub, camera not
  linked, 0x3bd40) and camera (real driver, links, 0xd9a90).
- The capture→transport→AI→audio path above the HAL is already proven on host,
  so bring-up is reduced to: confirm pins, init, capture — then it feeds an
  already-working pipeline.


---

<!-- ===== consolidated from: docs/AUDIO_BRINGUP.md ===== -->

# AUDIO_BRINGUP.md

Day-one plan for audio output (MAX98357A I2S amp) on the ordered board:
**ESP32-S3 N16R8 CAM + OV5640**.

Status: driver written (`firmware/src/hal/esp32/audio_esp.c`), compiles and links
against ESP-IDF v5.2 `i2s_std` — alone, and together with the camera driver —
**never run on silicon**. Mirrors `CAMERA_BRINGUP.md`.

Related: `HARDWARE_INTEGRATION_PLAN.md` §3 (audio), `audio.h` (the contract),
`CAMERA_BRINGUP.md` (the camera pin map this must not collide with).

---

## 1. What the device does with audio

The glasses are "dumb" on audio by design: the **phone** renders speech to PCM
and streams it down over the proven audio downlink (`audio_begin` → PCM chunks →
`audio_end`, SHA-256 verified); the device reassembles it and calls
`sd_audio_play_pcm`. So this driver is just an **I2S transmit path**: 16-bit PCM
in, sound out of the MAX98357A. No codec, no DAC, no mic involvement.

Sample rate is whatever the phone announces in `audio_begin` (espeak currently
renders 22050 Hz mono); `sd_audio_init` reconfigures the I2S clock per turn if it
changes, so no rate is hard-coded.

---

## 2. I2S pin map — ⚠️ ASSUMPTION, verify before wiring

MAX98357A needs three signals (no MCLK, no data-in):

| Amp pin | Signal | GPIO (assumed) |
|---|---|---|
| BCLK | bit clock | **42** |
| LRC | word select (WS) | **41** |
| DIN | serial data | **40** |
| GAIN | gain select | strap in HW (not driven) |
| SD | shutdown/channel | tie high for always-on (not driven) |

These three were chosen to avoid every committed pin (next section). **Day-one
task: confirm GPIO 40/41/42 are actually broken out on this CAM board's headers**
— minimal S3-CAM boards expose few pins. If they are not available, the free
fallbacks are **47, 48, 21, 14** (see budget). Change the three `I2S_PIN_*`
defines in `audio_esp.c`.

---

## 3. Pin budget — the camera/I2S conflict analysis (the key risk)

The High risk from `HARDWARE_INTEGRATION_PLAN.md` is "DVP camera pins collide
with I2S audio pins." Here is the full ESP32-S3 N16R8 GPIO budget:

| GPIO | Used by | Free for audio? |
|---|---|---|
| 0 | strapping (boot) | no |
| 1, 2 | **free** (ADC1 — reserve for battery) | keep for battery ADC |
| 3 | strapping (JTAG sel) | avoid |
| 4,5,6,7,8,9,10,11,12,13 | **camera** DVP/SCCB | no |
| 14 | free | yes (fallback) |
| 15,16,17,18 | **camera** XCLK/D7/D6/D5 | no |
| 19, 20 | native USB D-/D+ | no |
| 21 | free | yes (fallback) |
| 26–32 | SPI flash | no |
| 33–37 | **octal PSRAM** (R8) | no |
| 38, 39 | free | yes |
| 40, 41, 42 | **chosen for I2S** | yes ← BCLK/WS/DIN |
| 43, 44 | UART0 console | no |
| 45, 46 | strapping | no |
| 47, 48 | free | yes (fallback) |

**Conclusion: with the camera on GPIO ≤18 and I2S on 40/41/42, there is no pin
collision** — the two subsystems live on opposite ends of the GPIO range. The
only open question is *physical breakout* on this specific board, not a logical
clash. Battery ADC (priority #2, later) fits on GPIO 1 or 2 (ADC1; ADC2 is
unusable with Wi-Fi active). Spare after camera + I2S + battery: 14, 21, 38/39,
47, 48 — enough for a button and future needs.

> If the verified camera pin map turns out to use any of 40/41/42 (some board
> variants differ), move I2S to 47/48 + 21 and re-verify. This is exactly why the
> camera pin map must be confirmed *before* wiring audio.

---

## 4. Likely bring-up failure modes

| Symptom | Most likely cause | Fix |
|---|---|---|
| `I2S init failed` | Pin not exposed / invalid GPIO | Verify breakout; pick a free pin (§3) |
| Silence, init OK | DIN/BCLK/WS mis-wired or SD pin low | Check 3 wires; tie amp SD high |
| Loud static / noise | WS and BCLK swapped | Swap LRC/BCLK wiring |
| Distorted / too quiet/loud | GAIN strap / clipping | Set GAIN resistor; check PCM level |
| Pitch too high/low | Sample rate mismatch | Confirm device inits to `audio_begin` rate |
| Underruns / choppy | Write starves / task priority | PCM is fully buffered before play; raise task prio if needed |
| Reset on play | Current spike (amp + Wi-Fi) | Power from USB; add bulk capacitance; measure |
| Buzz only when camera active | Shared/adjacent noisy pin | Re-check pin map vs §3; keep I2S away from PCLK |

---

## 5. Day-one bring-up checklist (ordered)

1. **Confirm the camera pin map first** (`CAMERA_BRINGUP.md` §2) — audio pins
   must avoid whatever the camera actually uses.
2. **Confirm GPIO 40/41/42 are broken out** and free on the board; otherwise pick
   fallbacks (§3) and update `I2S_PIN_*`.
3. **Wire the MAX98357A:** BCLK→42, LRC→41, DIN→40, VIN→5V/3V3, GND→GND, SD→VIN
   (always on), speaker to the amp output. Confirm no I2S wire shares a camera pin.
4. **Build + flash with the audio driver:**
   `idf.py build -DSUDONIT_AUDIO_DRIVER=1 flash monitor`. (Add
   `-DSUDONIT_CAMERA_DRIVER=1` once the camera is up — they coexist.)
5. **Smoke-test tone:** the simplest first check is a known PCM buffer; then run
   the full path — `python -m phone.server` + an uplink — and confirm the spoken
   answer comes out of the speaker (this is the **AI→audio success criterion**).
6. **Tune** gain/volume and measure current draw (brownout risk) only after sound
   is clean.

Build flags: `-DSUDONIT_AUDIO_DRIVER=1` (I2S amp), `-DSUDONIT_CAMERA_DRIVER=1`
(OV5640), `-DSUDONIT_NET_SELFTEST=1`, `-DSUDONIT_PROVISION_CONSOLE=1`. All off by
default.

---

## 6. What is already done (firmware side)

- Real MAX98357A I2S-TX driver (ESP-IDF v5.2 `i2s_std`) implementing the
  `audio.h` contract (16-bit PCM, mono/stereo, per-turn rate reconfigure), behind
  `SUDONIT_AUDIO_DRIVER`. No fake sound.
- Build verified three ways: default (stub, 0x3bd40), audio-only (links), and the
  full demo config camera+audio+net (0xd9a90) — they coexist with no pin or build
  conflict.
- The phone→PCM→`sd_audio_play_pcm` path is already proven end-to-end on host
  (mock audio backend), so bring-up reduces to: pick/verify 3 pins, wire, play.


---

<!-- ===== consolidated from: docs/PROVISIONING_PLAN.md ===== -->

# PROVISIONING_PLAN.md

How configuration (Wi-Fi credentials, server address, device name) gets onto the
glasses — on the host today, and on the ESP32 when hardware arrives.

Status: serial provisioning **implemented** (host + ESP-IDF); BLE/SoftAP still design.
Related: the config subsystem (firmware/include/sudonit/config.h), the serial
console (firmware/include/sudonit/provisioning.h), DECISIONS.md (BLE = control
plane, Wi-Fi = data plane).

---

## The problem

The config subsystem can *store* and *read* credentials (NVS on device), but a
real pair of glasses has no keyboard or screen. We need a way to get the Wi-Fi
SSID/password and the server address **into** the device the first time, and to
re-provision or reset later.

On the **host build** this is already solved: `device_config set <key> <value>`
writes the same config the firmware reads. The options below are for the
**on-device** flow.

---

## Option A — BLE provisioning

A phone app connects over BLE and writes the credentials.

- **Fit:** BLE is already the chosen control-plane transport (DECISIONS.md), and
  the companion app is the natural provisioning UI. ESP-IDF ships
  `wifi_provisioning` with a BLE transport (`scheme_ble`) and a documented
  security mode (Security 1/2: X25519 key exchange + AES-CTR, optional proof-of-
  possession).
- **Pros:** no extra radio mode; same channel as normal control; good UX (scan,
  pick network, type password in the app); encrypted by the provisioning scheme.
- **Cons:** requires the companion app to implement the provisioning protocol;
  BLE bonding/pairing UX to get right.

## Option B — SoftAP provisioning

The device starts its own Wi-Fi access point; the user connects a phone/laptop
to it and submits credentials via a captive page or the ESP-IDF SoftAP scheme.

- **Fit:** works without the companion app (any browser), good fallback.
- **Pros:** universal client (browser); ESP-IDF `scheme_softap` is supported;
  no BLE stack needed for provisioning.
- **Cons:** spins up Wi-Fi AP mode (power, complexity, coexistence with the data-
  plane STA); clunkier UX (leave your network, join the device's AP, come back);
  captive portals are finicky across platforms.

## Option C — Serial provisioning  ✅ implemented

Credentials entered over the USB-C serial console (the same console used for
logs), via a small command interface (mirroring the host `device_config`).

- **Fit:** ideal for **bench/bring-up and recovery**, not for end users.
- **Pros:** trivial to implement (reuse the `sd_config_set_field` logic over a
  UART command loop); no radio; always available while debugging; perfect for
  the first hardware bring-up before BLE/SoftAP exist.
- **Cons:** requires a cable and a host terminal — not a consumer flow.

**This option is built** — see *Implemented serial flow* below.

---

## Recommended approach

**Phased:**

1. **Now / first bring-up — Serial.** Add a tiny serial command interface that
   reuses `sd_config_set_field` / `sd_config_get_field` (already built and
   tested). It is the fastest path to a provisioned device on the bench and
   doubles as the recovery channel. Zero new radio code.
2. **V1 consumer — BLE provisioning** via ESP-IDF `wifi_provisioning`
   (`scheme_ble`, Security 2). It aligns with the BLE control-plane decision and
   the companion-app direction, and gives an encrypted, app-driven flow.
3. **Fallback — SoftAP**, added only if BLE provisioning proves unreliable across
   target phones.

Rationale: serial unblocks hardware bring-up immediately with no radio work;
BLE is the right consumer flow and reuses an existing decision; SoftAP is a
hedge, not a default. All three write through the **same config API**, so the
storage/versioning/validation already built is reused unchanged.

---

## Implemented serial flow

Step 1 of the phased plan is done. The serial console is a
**transport-independent command processor** plus a thin REPL:

- **Module:** `firmware/include/sudonit/provisioning.h`,
  `firmware/src/app/provisioning.c`.
- **Design seam:** `sd_provision_handle_line(session, line, out, ctx)` takes one
  input line and emits output through a sink callback — **no I/O assumptions**.
  This is what makes it testable without hardware and reusable over any byte
  stream. `sd_provision_repl(FILE *in, FILE *out)` is the only I/O-bound piece;
  it works on host stdin/stdout and on the ESP32 UART (ESP-IDF maps stdio to the
  console UART) with the *same* code.
- **Config reuse:** every mutation goes through `sd_config_set_field` /
  `sd_config_get_field` / `sd_config_save` / `sd_config_defaults` — no parsing or
  validation is duplicated. A `session` holds an in-memory `sd_config_t` and a
  `dirty` flag; `save` is explicit so edits can be reviewed before persisting.

**Commands:**

| Command | Effect |
|---|---|
| `help` | list commands and valid keys |
| `show` | print all settings (password masked) |
| `get <key>` | print one setting (password → `(set)`/`(unset)`) |
| `set <key> <value>` | change one setting in memory (marks dirty) |
| `save` | persist the session to storage (NVS on device) |
| `reset` | restore built-in defaults (then `save` to persist) |

Keys: `device_name | wifi_ssid | wifi_password | server_host | server_port`.

**Password handling (enforced + tested):** the Wi-Fi password is **never echoed**.
`show` and `get wifi_password` report only `(set)`/`(unset)`; `set wifi_password`
acknowledges with `ok: wifi_password updated` and never repeats the value. The
`set` value is taken as the line remainder, so passwords containing spaces are
preserved verbatim. A host test asserts the cleartext never appears in any output.

**Where it runs:**

- **Host:** `device_provision` (`firmware/src/app/main_provision.c`) — pipes or
  interactive, e.g. `printf 'set server_host 10.0.0.2\nsave\n' | device_provision`.
- **ESP32:** compiled into the `main` component; the REPL is invoked from
  `app_main` behind the `SUDONIT_PROVISION_CONSOLE` build flag so production
  boots are unaffected while bench/recovery builds get the console over UART.

**Tests:** `firmware/test/test_provisioning.c` (ctest suite `provisioning`)
drives `handle_line` through a capturing sink — help/show/get/set/save/reset,
password masking, password-with-spaces, persistence across a reload, and the
error paths (unknown command, invalid key, out-of-range port, blank line).

---

## Security considerations

- **Never log the password.** Enforced today: `app_main`, `device_config`, and
  the serial provisioning console (`device_provision`) print `(set)/(unset)`,
  never the value — and the provisioning suite tests that the cleartext never
  appears in any command output.
- **Encrypt the provisioning channel.** Use ESP-IDF provisioning Security 2
  (SRP6a) or Security 1 (X25519+AES-CTR) with a per-device proof-of-possession,
  so credentials aren't sent in the clear over BLE/SoftAP.
- **Credentials at rest.** Wi-Fi credentials live in NVS. Plan to enable **NVS
  encryption** (flash-encryption-backed) before shipping; until then, treat a
  provisioned device as holding a recoverable secret.
- **No secrets in logs, events, or crash dumps.** Keep the password out of any
  telemetry/diagnostics payload (future telemetry subsystem must honor this).
- **Authenticate the data plane separately.** Provisioning sets *network*
  credentials; the glasses↔phone trust (pairing/auth for the control + data
  planes) is a separate concern (SECURITY model, future doc).

---

## Recovery / reset flow

- **Re-provision:** `set <key> <value>` + `save` over the serial console (or BLE
  later) overwrites the stored config — no wipe needed for a simple network
  change.
- **Recovery channel:** the serial console is **built and available today**
  (`SUDONIT_PROVISION_CONSOLE` build) over USB-C even if Wi-Fi/BLE config is
  wrong, so a misconfigured device is never bricked. `reset` restores defaults
  from the same console.
- **Boot fallback:** if `sd_config_load` finds no/invalid config, it already
  returns defaults (empty credentials) and the device boots; the firmware should
  then enter a provisioning-needed state rather than crash-loop.

## Factory reset behavior

- **Trigger (planned):** a long-press on the device button at boot (debounced),
  or a serial `factory-reset` command. (Button HAL is a future addition.)
- **Action:** erase the `sudonit` NVS namespace (or `nvs_flash_erase()` for a
  full wipe), returning config to built-in defaults; log
  `factory reset — configuration cleared` (no secrets).
- **Result:** the device reboots unprovisioned and awaits provisioning again.
- **Out of scope here:** clearing any future user data / paired-device trust —
  that belongs to the security/storage subsystems and should be wiped by the
  same factory-reset action when those exist.


---

<!-- ===== consolidated from: REFERENCES.md ===== -->

# REFERENCES.md

# Purpose

This file stores useful references, hardware choices, datasheets, repositories, articles and research.

It is the project's memory.

---

# Hardware

## Main Controller

ESP32-S3

Current Target:

ESP32-S3 N16R8

Notes:

* Wi-Fi
* Bluetooth
* Good community support
* AI companion architecture

---

## Camera

Current Target:

OV5640

Reasons:

* Better image quality
* Good ESP32 compatibility
* Suitable for AI image analysis

Alternative:

OV2640

---

## Microphone

Preferred:

ICS43434

Alternative:

INMP441

Interface:

I2S

---

## Audio

Current Direction:

Open-ear speaker

Reason:

* Smaller than bone conduction
* Better audio quality
* Easier integration

Amplifier:

MAX98357A

---

## Battery

Current Experiments:

502080 1000mAh

Potential Layout:

Left arm:
Battery

Right arm:
Battery + Electronics

Benefits:

* Weight balancing
* Longer runtime

---

## Charging

TP4056 USB-C

Requirements:

* Battery protection
* USB-C connector

---

# Software

## Smartphone Companion

Platform:

Android

Responsibilities:

* AI communication
* Cloud access
* Notifications
* Settings
* Storage

---

## Firmware

Platform:

ESP32-S3

Responsibilities:

* Camera control
* Audio
* Communication
* Sensors
* Display

---

# Future Research

## Display

Research Required:

* SPI displays
* OLED microdisplays
* HDMI microdisplays
* AR optics

Status:

OPEN

---

## Optics

Research Required:

* Prism systems
* Birdbath optics
* Waveguides

Status:

OPEN

---

# Repositories

Useful repositories should be added here.

Template:

Name:
URL:
Purpose:
Notes:

---

# Articles

Useful articles should be added here.

Template:

Title:
URL:
Summary:

---

# Lessons Learned

Important discoveries should always be documented here.

Reason:

Future Alex will forget.
