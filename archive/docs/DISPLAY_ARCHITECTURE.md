# DISPLAY_ARCHITECTURE.md

The display subsystem design — **interfaces and data flow only**, no panel-specific
code.

## 0. Governing principle (read this first)

> **Everything that can reasonably run on the glasses should run on the glasses.
> External devices extend the system; they do not replace it.**

Sudonit is a **standalone device that becomes more powerful when connected** to a
phone, a server, or an AI provider — *not* a thin client that is a black screen
without one. The display subsystem therefore puts the **rendering engine on the
glasses**. The device owns its framebuffer, fonts, layout, widgets, and screens, and
draws its own pixels from local state. The phone sends **content and data** (an AI
answer, a route, a notification payload, calendar events to sync) — never a stream of
pixels for the device to dumbly blit.

A bitmap-push path still exists, but only as a deliberate **escape hatch** for the
rare thing the device genuinely cannot render itself (a photo thumbnail, a complex
chart). It is the exception, not the architecture.

This document supersedes the earlier "phone renders, device blits" framing. Where it
diverges from the older top-level `ARCHITECTURE.md` (which assigned settings/storage
to the phone), **this document is the current intent** for the display/UI layer; that
file should be reconciled to match.

We explicitly **do not optimize for minimum firmware complexity.** We optimize for
**device autonomy and long-term product independence.** The extra on-device code is
the point, not a cost to be minimized.

---

## 1. Design principles

1. **The device renders itself.** Layout, fonts, widgets, and screens live in
   firmware. Given local state, the glasses produce a correct, useful UI **with no
   phone present**.
2. **The phone extends, it does not power.** It provides AI, navigation routing, cloud
   sync, large-scale processing, and rich integrations — as *data*, which the device
   renders locally. Disconnect the phone and the device stays a functioning wearable.
3. **Local-first state.** Clock, calendar, settings, battery, and notifications are
   owned and persisted **on the device**. The phone *syncs* and *enriches* this state;
   it is not the source of truth for it.
4. **Semantic content over pixels.** The wire carries meaning (`notify`, `ai_answer`,
   `nav_route`, `calendar_sync`), not framebuffers. This is smaller, more resilient to
   disconnects, and lets the device render in its own style/fonts/panel format.
5. **Bitmap push is an escape hatch, not a foundation.** Available for genuinely
   un-renderable rich content; never the default for text/UI.
6. **Partial, local updates.** The device redraws only dirty regions of its own
   framebuffer when local state changes (a ticking clock, an arriving notification).
   No network round-trip is needed to update on-device content.
7. **Monochrome-first, format-aware.** V1 is 1-bit; the render engine targets a
   `DisplayProfile` so 4-bit/RGB565 panels need no rewrite (DISPLAY_EVALUATION.md).

---

## 2. Two-tier model: on-device runtime vs phone extensions

```
  GLASSES  (autonomous — works with no phone)        PHONE / SERVER  (extends)
  ┌───────────────────────────────────────────┐     ┌──────────────────────────┐
  │  Screen manager + input router            │     │  AI provider (answers)   │
  │   ├─ Home / clock screen                  │     │  Navigation routing      │
  │   ├─ Notifications screen                 │ ◄── │  Calendar/cloud sync      │
  │   ├─ Calendar / agenda screen             │data │  Large-scale processing  │
  │   ├─ Camera controls screen               │ ──► │  Rich integrations       │
  │   ├─ Settings screen                      │(sync)│  Settings backup         │
  │   └─ AI / assistant screen                │     └──────────────────────────┘
  │            │                              │
  │  Local services: clock(RTC), calendar     │     The link is OPTIONAL.
  │   store, settings(NVS), battery mgmt,      │     Present → richer. Absent →
  │   notification store, camera control       │     the device still works.
  │            │                              │
  │  UI runtime: widgets → layout → raster     │
  │            │                              │
  │  Framebuffer → sd_display (HAL) → panel    │
  └───────────────────────────────────────────┘
```

The **left box is the product.** The right box makes it better. The arrows carry
structured data and sync, in both directions, over the existing transport.

---

## 3. On-device UI runtime (the core of this subsystem)

A real, if small, rendering stack in firmware. Layered so each piece is replaceable
and testable on the host (it compiles in the host build with a memory-backed panel,
exactly like the rest of the HAL):

1. **Raster core** — a framebuffer plus primitives (pixel, line, rect, blit) and a
   bundled bitmap font with glyph metrics. Deterministic → snapshot-testable.
2. **Widgets** — reusable elements: text label (with wrap + scroll), icon, gauge/bar,
   list row, banner, big-number, arrow. Each draws itself into a rect.
3. **Layout** — places widgets into the active `DisplayProfile`'s canvas with safe
   eyebox margins; handles wrap/clip/scroll for a tiny panel.
4. **Screen manager** — owns the set of screens (§4), the active screen, transitions,
   and a reserved system status strip (clock/battery/link) present on every screen.
5. **Input router** — maps the physical button(s)/gesture into navigation
   (next/select/back) and per-screen actions. Input drives the UI locally with no
   phone in the loop.

The render engine redraws on **local state change** (a service updates a value) or
**input**, emitting dirty rects to the panel. The phone is not involved in a redraw.

---

## 4. Local services & screens (run on the glasses)

Each is an on-device module owning its state, persisting it locally, and feeding a
screen. The phone may sync/enrich each, but none *depends* on the phone to function.

| Service / screen | Runs on glasses | Local store | Phone extends with |
|---|---|---|---|
| **Clock / home** | timekeeping, alarms, display | RTC + NVS | time sync, timezone |
| **Calendar / agenda** | store + render upcoming events | flash cache | event sync from phone calendar |
| **Notifications** | receive, store, view, dismiss | ring buffer in flash | new notifications relayed from phone |
| **Settings** | edit + persist device config | NVS | optional cloud backup/restore |
| **Battery management** | gauge, charge state, low-power policy | live + NVS | usage stats, health history |
| **Camera controls** | shutter, mode, resolution, preview | NVS | uploads, AI capture requests |
| **AI / assistant** | render answers, show "thinking" | transient | the AI answer itself (phone/cloud) |
| **Navigation** | render arrow/distance/street, count down | route in RAM | the route + reroutes (phone) |

Key consequence: **clock, calendar, settings, battery, notification viewing, and
camera control all work with the phone switched off.** Navigation and AI are the two
screens that need the phone to *populate* — but even they render on-device from the
last data received (e.g. the current route keeps counting down through a brief
dropout).

---

## 5. Phone/server extension protocol (data & sync, not pixels)

The phone pushes **structured content**; the device persists and renders it. Reuses
the existing length-prefixed framing (`[kind:1][len:uint32 BE][payload]`, `'J'` JSON /
`'B'` binary) — a new family of message *types*, no new transport.

Representative content messages (phone → device):

```json
{ "type": "ai_answer",  "id": 42, "text": "That's a snake plant — water every 2–3 weeks." }
{ "type": "notify",     "app": "messages", "title": "Mara", "body": "see you at 6?", "icon": 3, "ts": 1718800000 }
{ "type": "nav_route",  "dest": "De Hoek café",
  "steps": [ { "arrow": "right", "street": "Voorstraat", "dist_m": 220 }, ... ] }
{ "type": "calendar_sync", "events": [ { "ts": 1718812800, "title": "Dentist", "loc": "Centrum" } ] }
{ "type": "time_sync",  "epoch": 1718800000, "tz": "Europe/Amsterdam" }
{ "type": "settings_restore", "kv": { "brightness": 7, "hand": "left" } }
```

Device → phone (the device is an active participant, not a screen):

```json
{ "type": "capture_request", "reason": "user_pressed_ask" }   // device asks phone to run AI on a frame
{ "type": "settings_backup", "kv": { ... } }                  // device owns settings, phone stores a copy
{ "type": "notify_dismissed", "id": 17 }                      // keep phone/device state consistent
{ "type": "hello", "display": { "present": true, "w": 256, "h": 64, "formats": ["mono1"] },
                   "caps": ["clock","calendar","notify","camera","nav","ai"] }
```

The device advertises its on-board capabilities in `hello` so the phone knows what it
can extend. A capability the device handles locally (clock) is *synced*; a capability
it cannot do alone (ai, nav) is *provided*.

### 5.1 Bitmap escape hatch (rare, optional)

For content the device genuinely cannot render, the old bitmap downlink remains
available — `display_begin` (JSON header: `frame_seq`, `format`, `canvas`, `rects`,
`size`, `chunks`) → `display_chunk` (binary `[seq][packed bytes]`) → `display_end`
(SHA-256), reassembled and SHA-verified like the audio downlink. **It targets a
single region of a single screen and is never used for text or standard UI.** If you
find yourself reaching for it to draw a label, the renderer is missing a widget —
add the widget instead.

---

## 6. Refresh strategy

Driven by **local state**, not the network:

| Tier | Trigger | Source | Example |
|---|---|---|---|
| **Local tick** | on-device timer | device | clock minute, nav distance countdown |
| **Local event** | service state change | device | button press, battery threshold, alarm |
| **Sync event** | content arrives from phone | phone | new notification, AI answer, reroute |
| **Full redraw** | screen switch / resync | device | navigating between screens |

A redraw is a local operation: the active screen re-lays-out and pushes dirty rects to
the panel. **Bandwidth to the phone is decoupled from frame rate** — the panel can
update many times a second (a smooth countdown) while the phone sent one small `nav_route`
message minutes ago.

---

## 7. Device-side HAL & layering (interfaces only)

Two layers. The low-level panel HAL is the only panel-specific code; the UI runtime
sits above it and is panel-agnostic.

```c
/* sudonit/hal/display.h — low-level panel I/O (backend-swappable: host mem, SPI OLED, …) */
typedef struct { uint16_t width, height; uint8_t format; } sd_display_info_t;
sd_err_t    sd_display_init(void);
sd_err_t    sd_display_info(sd_display_info_t *out);
sd_err_t    sd_display_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                            const uint8_t *packed, size_t len);   /* dirty rect → panel */
sd_err_t    sd_display_present(void);
const char *sd_display_backend(void);                              /* "ssd1306" | "memfb" | "stub" */
```

```c
/* sudonit/ui/*.h — the on-device runtime (panel-agnostic, host-testable) */
sd_ui_init(); sd_ui_tick();                       /* drive local timers/animation */
sd_ui_input(SD_UI_NEXT | SD_UI_SELECT | SD_UI_BACK);
sd_ui_screen_show(SD_SCREEN_HOME | NOTIFY | CALENDAR | CAMERA | SETTINGS | AI | NAV);
/* content ingress from the phone-extension client */
sd_ui_notify_push(const sd_notify_t*);  sd_ui_ai_answer(const char*);
sd_ui_nav_route(const sd_nav_route_t*); sd_ui_calendar_sync(const sd_event_t*, size_t);
```

The default `sd_display` backend is an **honest stub** (`SD_ERR_UNSUPPORTED`, no fake
pixels); the host build uses a **memory-framebuffer** backend so the entire UI runtime
runs and is snapshot-tested with no hardware. The SPI-OLED backend is written when V1
hardware arrives — and, per the milestone rule, *that* panel driver is exactly the
firmware that reduces real display-bring-up risk.

Both layers live behind a `SUDONIT_DISPLAY` build flag so a displayless/audio-only
build stays lean — but note that "lean" no longer means "dumb": even without a panel,
the local services (clock, calendar, settings, battery, notifications) run; they
simply have no screen to draw on.

---

## 8. Degradation: what happens with no phone

| State | Behavior |
|---|---|
| Phone connected | Full system: local UI + AI + navigation + live notifications + sync |
| Phone disconnects mid-use | Current route keeps counting down; last AI answer stays; clock/calendar/settings/battery/notification history all work |
| Never paired | A functional wearable: clock, alarms, calendar (once ever synced), settings, battery, camera capture to local storage |
| No display fitted (audio-only V1) | Local services still run; output is audio + capture; UI is headless |

This table is the whole point of the rewrite: **there is no state in which the device
becomes a black, useless screen because the phone is away.**

---

## 9. What this design deliberately does **not** do

- It does **not** make the phone render pixels for normal UI (the escape hatch aside).
- It does **not** treat the phone as the source of truth for clock/calendar/settings/
  notifications/battery — those are device-owned and merely synced.
- It does **not** require the phone for the device to be useful.
- It does **not** put AI inference, route *computation*, or heavy cloud work on the
  device — those legitimately exceed the glasses and are the phone's job to *provide*.
  (Autonomy means owning what's reasonable locally, not pretending an ESP32 is a GPU.)
- It does not specify panel timing/init — that lives in a future `DISPLAY_BRINGUP.md`
  against the real V1 panel, mirroring `CAMERA_BRINGUP.md`.

---

## 10. Test strategy (all host-side, no glasses)

- **Raster/widgets/layout/screens**: golden-image snapshot tests against the memory-
  framebuffer backend (`state → screen → raster` → PNG diff), like `tests/test_image.py`.
- **Local services**: unit tests for clock/calendar/notification/settings/battery logic
  with no phone — proving autonomy directly.
- **Extension protocol**: Python encoder ↔ C decoder round-trip over a socketpair for
  each content/sync message, like the audio-downlink interop test.
- **Disconnect behavior**: simulate phone dropout and assert the device keeps rendering
  from local state (§8) — the headline guarantee, tested explicitly.

The full UI and all local services are therefore provable on the host before any panel
exists — the same pre-hardware discipline as the rest of the project, now applied to a
device that stands on its own.
