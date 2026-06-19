# DEMO_METRICS.md

The exact measurements to collect the day hardware arrives. The purpose is narrow and
deliberate: **replace opinions with numbers.** Every claim in the audit (`PROJECT_AUDIT.md`,
`TOP_RISKS.md`) about latency, power, and image quality is currently a guess; this document
is the instrument list that turns each guess into a recorded figure.

This adds **no features and no architecture** — it only defines what to measure, against
what thresholds, with what method. Run it alongside `DEMO_DAY_PLAN.md` (same step order).

## How to use this

- Fill the **Result** column on the bench. One number per metric, plus a pass/fail.
- Thresholds: **Target** = the product would feel good · **Acceptable** = usable, note it ·
  **Failure** = stop and fix before moving on; this is a real problem, not a tuning detail.
- Prefer the **simplest honest method**: a phone stopwatch and a USB power meter beat
  nothing. Note the method actually used next to each result.
- If a metric can't be measured yet (e.g. no battery cell present), write **N/A — reason**.
  A documented gap is a result; a blank is not.

### Bench instruments needed
USB-C power meter (inline, mA + mWh), a phone/stopwatch, the host machine for timestamp
logging, the seller's camera pin diagram, a known 2.4 GHz AP, `ANTHROPIC_API_KEY` exported,
and (optionally) a LiPo + charger. Where possible, log timestamps in firmware/server over
serial so timing isn't eyeballed.

---

## 1. Boot time
- **What:** power-on / reset → app_main running and the boot/HAL log printed (default build).
- **Target:** ≤ 2 s · **Acceptable:** ≤ 5 s · **Failure:** > 10 s or boot loop.
- **Method:** timestamp from reset edge to first app log line over serial monitor; average
  of 3 cold boots. (For a wearable, also note wake-from-idle later, but day one is cold boot.)

## 2. Wi-Fi connect time
- **What:** `sd_net_start` called → STA associated + IP acquired (ready to TCP-connect).
- **Target:** ≤ 3 s · **Acceptable:** ≤ 8 s · **Failure:** > 15 s or intermittent (< 90%
  of attempts associate).
- **Method:** firmware timestamps association-start → `GOT_IP`; run 10 attempts, record
  mean and success rate. Use a controlled 2.4 GHz AP. **Never log the password.**

## 3. Capture latency (device-local)
- **What:** capture trigger → JPEG frame available in PSRAM (camera only, no network).
- **Target:** ≤ 200 ms · **Acceptable:** ≤ 500 ms · **Failure:** > 1 s, or frame size
  implausibly small / decode fails.
- **Method:** firmware timestamps `esp_camera_fb_get()` call → return; record frame byte
  size too. 10 captures; report mean + min/max. Note resolution (SVGA unless changed).

## 4. AI round-trip latency (the headline number)
- **What:** capture trigger → AI answer text received back on the device (full loop:
  capture → ESP32 → Wi-Fi → phone → Claude → answer → device). Audio excluded (metric 5).
- **Target:** ≤ 2 s · **Acceptable:** ≤ 4 s · **Failure:** > 6 s, or > 1-in-10 requests
  error/timeout.
- **Method:** single clock — log `t0` at trigger on the device, `t1` at `ai_response`
  receipt; the phone server logs its own send/receive + the provider call duration so the
  total can be decomposed into **on-device + network-to-phone + provider** (knowing *where*
  the seconds go matters as much as the total). 20 queries over a normal connection; report
  median, p90, and the decomposition. Also record **provider cost per query** if available.

## 5. Audio latency
- **What:** AI answer text ready on device → first audible sample from the speaker (TTS on
  phone → PCM downlink → `sd_audio_play_pcm`).
- **Target:** ≤ 500 ms · **Acceptable:** ≤ 1 s · **Failure:** > 2 s, audible gaps/
  underruns, or buzz when the camera is active.
- **Method:** timestamp answer-ready → first I2S write; confirm audibly. Separately note
  any **camera-active buzz** (PCLK coupling) as pass/fail. 10 playbacks.

## 6. Idle current
- **What:** average supply current with the device booted, Wi-Fi associated, display (if
  any) on its idle screen, no capture/audio.
- **Target:** ≤ 80 mA · **Acceptable:** ≤ 150 mA · **Failure:** > 250 mA, or unstable.
- **Method:** inline USB power meter, 60 s average after settling. Record with Wi-Fi on and
  (separately) Wi-Fi off to quantify the radio's idle cost — this directly informs the
  "Wi-Fi is the wrong transport" risk (TOP_RISKS #13).

## 7. Active current (peak + sustained)
- **What:** supply current during a full capture → uplink → audio cycle; capture both the
  **peak** (the brownout risk) and the **sustained** draw during Wi-Fi TX + audio.
- **Target:** peak ≤ 500 mA, sustained ≤ 300 mA · **Acceptable:** peak ≤ 800 mA ·
  **Failure:** peak causes a **brownout/reset**, or sustained > 1 A.
- **Method:** USB meter with peak-hold (or a scope on a shunt if available) across 10 full
  cycles; note any reset. **This is the single most important power number** — it decides
  whether the device survives camera+Wi-Fi+audio together (TOP_RISKS #9).

## 8. Battery life estimate
- **What:** projected runtime from a representative usage mix, derived from metrics 6–7.
- **Target:** ≥ 4 h mixed use · **Acceptable:** ≥ 1 h · **Failure:** < 30 min, or thermal
  (enclosure/temple uncomfortably warm).
- **Method:** if a cell is present, measure mWh draw over a 10-min scripted mix (e.g. 1
  query/min + idle) and extrapolate to the cell's capacity; also do a real drain-to-cutoff
  if time allows. If no cell: compute the estimate from metrics 6–7 and a candidate mAh,
  and mark **estimate (no cell tested)**. Record surface temperature after 10 min of active
  use regardless.

## 9. Image quality checks
- **What:** is the captured frame good enough for reliable AI answers? Qualitative but
  recorded against fixed checks, not vibes.
- **Target:** ≥ 8/10 checks pass · **Acceptable:** ≥ 6/10 · **Failure:** ≤ 4/10, or the AI
  visibly misreads typical scenes.
- **Method:** capture a fixed test set (a printed paragraph at ~1 m, a street-sign-like
  high-contrast text, a colored object, an indoor scene, a backlit scene) and score each:
  | Check | Pass if |
  |---|---|
  | Exposure (indoor) | subject neither blown out nor crushed |
  | Exposure (backlit) | subject still legible |
  | Focus / sharpness | printed text edges crisp at ~1 m |
  | Readable text | a short printed paragraph is human-readable in the JPEG |
  | Color fidelity | a known-colored object is recognizably that color |
  | Motion (slight) | a small head-turn doesn't smear the frame unusably |
  | Field of view | the intended subject is actually framed |
  | JPEG integrity | decodes cleanly, no corruption/artifacts |
  | File size sane | not implausibly tiny/huge for the scene |
  | **AI agreement** | Claude's answer about the frame is correct for ≥ 4/5 test scenes |
  The last row is the one that matters most: it tests the *whole* value proposition on a
  real frame (TOP_RISKS #7, #21).

---

## Results table (fill on the bench)

| # | Metric | Target | Acceptable | Failure | Result | Pass? | Method/notes |
|---|---|---|---|---|---|---|---|
| 1 | Boot time | ≤2 s | ≤5 s | >10 s | | | |
| 2 | Wi-Fi connect | ≤3 s | ≤8 s | >15 s / <90% | | | |
| 3 | Capture latency | ≤200 ms | ≤500 ms | >1 s | | | |
| 4 | AI round-trip | ≤2 s | ≤4 s | >6 s / >10% err | | | median + p90 + split |
| 5 | Audio latency | ≤500 ms | ≤1 s | >2 s / buzz | | | |
| 6 | Idle current | ≤80 mA | ≤150 mA | >250 mA | | | Wi-Fi on/off |
| 7 | Active current | peak ≤500 mA | peak ≤800 mA | brownout | | | **peak-hold** |
| 8 | Battery life | ≥4 h | ≥1 h | <30 min | | | + temp |
| 9 | Image quality | ≥8/10 | ≥6/10 | ≤4/10 | | | scene scores |

---

## The four numbers that decide everything

If the day produces nothing else, get these — they are the figures the audit says do not
exist and that separate "product" from "project":

1. **Active peak current** (#7) — does it brown out? (reliability)
2. **AI round-trip median + p90** (#4) — does the magic feel instant or broken? (UX)
3. **AI-agreement on real frames** (#9, last row) — is the answer actually right? (value)
4. **Battery life / draw** (#8) — can it be worn off a wall socket? (wearability)

Record those four before touching the display, the UI runtime, or anything cosmetic.
