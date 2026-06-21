# DISPLAY_BRINGUP_PLAN.md

A **hardware-agnostic** plan for integrating a physical display, written so the *same*
UI runtime (UI_RUNTIME_ARCHITECTURE.md) lights up on:

- the **current ESP32-S3 prototype** (a small SPI mono OLED — DISPLAY_EVALUATION.md V1), and
- a **future Raspberry Pi / Linux-class device** (a DRM/SDL window or a larger panel),

by changing **only the bottom layer**. This is architecture and process, not firmware.
Panel-specific timing/registers go in a per-panel appendix when a real panel is chosen,
mirroring `CAMERA_BRINGUP.md` / `AUDIO_BRINGUP.md`.

---

## 1. The one rule that makes this portable

> Everything above the **`sd_display` HAL** (UI_RUNTIME_ARCHITECTURE.md §8.3) is
> platform-independent and already host-tested. Bringing up a display on any platform
> means implementing **one backend** behind that fixed interface — nothing in the
> apps, widgets, navigation, services, or state machine changes.

```
   apps / widgets / screens / services / state machine     ← identical everywhere
   Renderer (software rasterizer)  +  Font provider         ← identical (default)
   ─────────────────────────────────────────────────────────  ◀ PORT SEAM
   sd_display backend:  memfb | ssd1306(SPI) | drm | sdl     ← the ONLY new code
   physical panel
```

The contract to satisfy (already defined, do not change it):

```c
sd_err_t    sd_display_init(void);
sd_err_t    sd_display_info(sd_display_info_t *out);   /* w,h,format → DisplayProfile */
sd_err_t    sd_display_blit(uint16_t x,uint16_t y,uint16_t w,uint16_t h,
                            const uint8_t *packed, size_t len);   /* one dirty rect */
sd_err_t    sd_display_present(void);
const char *sd_display_backend(void);
```

A backend is "done" when it: initializes the panel, reports an accurate
`DisplayProfile`, blits a packed dirty-rect at (x,y), and presents — that's all the
runtime needs.

---

## 2. The DisplayProfile (how one runtime fits many panels)

`sd_display_info` returns a profile the layout engine adapts to (UI_RUNTIME §8.4):

```c
typedef struct {
  uint16_t  width, height;     /* logical pixels after rotation */
  sd_pixfmt_t format;          /* SD_PIXFMT_MONO1 | GRAY4 | RGB565 | RGB888 */
  sd_color_kind_t color;       /* mono | gray | color */
  uint8_t   rotation;          /* 0/90/180/270 */
  uint8_t   safe_margin;       /* eyebox / overscan inset in px */
} sd_display_info_t;
```

The runtime never hard-codes 256×64 or "mono". Containers, the status strip, and safe
margins scale from the profile, so a V1 OLED and an RPi panel run the same apps with no
per-screen edits. **This struct is the portability currency** — a new platform fills it
in and the UI adapts.

---

## 3. Backend responsibilities by platform (reference)

| Concern | `memfb` (host) | `ssd1306`/SPI OLED (ESP32 V1) | `drm`/`sdl` (RPi/Linux) |
|---|---|---|---|
| Transport | RAM buffer | SPI (esp_lcd or bit-bang) | DRM/KMS or SDL surface |
| Pixel format | matches target | MONO1 (packed pages) | RGB565/888 |
| `blit` | memcpy into canvas | push rect to GDDRAM window | copy rect into mapped fb / texture |
| `present` | no-op / dump PNG | flush / refresh | page-flip / SDL_RenderPresent |
| Init | alloc | panel reset+init seq, contrast | open card/window, set mode |
| Profile | configurable (tests) | 128×64/256×64 mono, rot | queried from mode/window |

The **software Renderer** writes the framebuffer for `memfb`, `ssd1306`, and a basic
`drm`/`sdl`; a Linux target *may* later swap in an accelerated Renderer, but is not
required to — the seam supports both.

---

## 4. Hardware-agnostic bring-up procedure (any panel)

Each step gates the next and localizes failure to one thing (the CAMERA/AUDIO bring-up
discipline):

1. **Pre-flight on host (`memfb`)** — confirm the UI runtime + state machine render
   correctly against the target's *DisplayProfile* (set memfb to the panel's w/h/format).
   Snapshot tests pass here **before touching hardware**.
2. **Backend skeleton** — implement `sd_display_*` for the panel returning a correct
   `DisplayProfile`; `blit`/`present` may no-op. Build links; runtime runs headless.
3. **Panel power + init** — bring the panel up to a known state (all-on / all-off test
   pattern straight from the backend, no runtime). Proves wiring/transport/init.
4. **Single dirty-rect blit** — push one known rect (a filled box) at a known (x,y).
   Proves addressing, byte order, and packing match the Renderer's output.
5. **Full present** — render one real screen (the boot/clock screen) through the whole
   stack. Proves Renderer→framebuffer→blit→present end to end.
6. **Dirty-rect correctness** — change one widget (clock minute) and confirm only its
   rect updates, in the right place. Proves partial updates (the smoothness/power win).
7. **Profile/rotation** — verify rotation, safe margins, and that layout fits the real
   panel (no clipping at edges / eyebox).
8. **Input loop** — bind the platform's input (button/encoder/keyboard) into logical
   actions (UI_RUNTIME §5) and walk the navigation model on the real panel.
9. **Soak** — leave it rendering (clock ticking, idle→sleep→wake) to catch refresh,
   ghosting, burn-in, and power issues.

Steps 1–2 are platform-independent; 3–9 differ only in the backend, not the runtime.

---

## 5. What each target needs beyond the display backend

Portability isn't only pixels — the runtime assumes three small platform services,
each already abstracted:

- **Timers** (run loop, clock tick, animation) — ESP-IDF FreeRTOS timers / Linux timerfd.
- **Persistent KV** (settings, stores) — NVS on ESP32 / a file on Linux, behind the
  existing config-store interface.
- **Input** — GPIO button on ESP32 / evdev or keyboard on Linux, behind the §5 input map.

A new platform implements these three + one display backend. Nothing else moves.

---

## 6. Risks & how the architecture contains them

| Risk | Containment |
|---|---|
| Panel byte order / packing mismatch | isolated to one backend; step 4 catches it before any UI |
| Slow full refresh (mono OLED over SPI) | dirty-rect blits (step 6) keep updates tiny |
| Different color depth later | `sd_pixfmt_t` + Renderer handle mono1/gray4/rgb565; no app change |
| Eyebox clipping (near-eye) | `safe_margin` in the profile; step 7 |
| Linux port drift | the host `memfb` backend is effectively the Linux port's twin; keeping CI green on memfb keeps the seam honest |
| Burn-in / ghosting | soak (step 9) + idle/sleep states (UI_STATE_MACHINE.md Region P) |

---

## 7. What this plan deliberately omits (until a panel is chosen)

- Exact init sequences, contrast/charge-pump values, SPI clock, pin mapping → a future
  per-panel appendix (e.g. `DISPLAY_BRINGUP_SSD1306.md`), like the camera/audio docs.
- Choice of `esp_lcd` vs. a minimal driver → a backend implementation detail.
- Optical alignment of the combiner → FRAME_ARCHITECTURE.md / hardware, not the runtime.

---

## What should be implemented first when a display is physically connected?

**A single `sd_display` SPI-OLED backend that lights one dirty rectangle — and nothing
above it.** Concretely, in order, smallest provable unit first:

1. **`sd_display` backend skeleton** for the connected panel: real `sd_display_info`
   (correct `DisplayProfile`), real `init`, and `blit`/`present` that push **one filled
   rectangle** to the glass (procedure steps 2–4). This is *the* first firmware to write
   for the display, and the only thing on the critical path — it proves wiring, init,
   addressing, byte order, and packing, which are the real unknowns.
2. **Render one real screen through the existing runtime** — the **Clock/boot screen**
   (step 5). It needs nothing from the phone, so it isolates the display from every other
   subsystem and immediately demonstrates *autonomy on glass*: the device shows the time,
   standalone.
3. **Dirty-rect update of the clock minute** (step 6) — proves partial refresh, the
   property everything else (smoothness, power) depends on.

Everything else — the full app set, navigation, overlays, the state machine — is already
written and host-tested against `memfb`, so once that backend blits correctly, the rest
"just renders." **Do not** start by porting apps or wiring the phone; start by making one
rectangle appear, then one local screen. The first light on the panel should be the
device's own clock, drawn by its own runtime, with no phone attached — the bring-up
that proves the whole autonomy-first thesis on real hardware.
