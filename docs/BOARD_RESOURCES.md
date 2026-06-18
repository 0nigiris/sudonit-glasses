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
