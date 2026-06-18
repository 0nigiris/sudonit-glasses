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
