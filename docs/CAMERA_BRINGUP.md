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
