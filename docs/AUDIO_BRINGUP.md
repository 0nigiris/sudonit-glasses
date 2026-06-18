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
