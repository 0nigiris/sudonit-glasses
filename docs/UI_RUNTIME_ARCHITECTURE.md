# UI_RUNTIME_ARCHITECTURE.md

The on-device UI runtime — the firmware that makes Sudonit an **autonomous device
that renders its own interface** (VISION.md Core Principle 1, DISPLAY_ARCHITECTURE.md
§0). The phone *extends* this runtime with content; it never owns it. The glasses lay
out, draw, and navigate their own screens from local state, with no phone present.

This is the detailed design of DISPLAY_ARCHITECTURE.md §3 ("on-device UI runtime").
It is **architecture only** — interfaces, layering, and data flow. No panel timing,
no register pokes. It is written to run on the ESP32-S3 today and to port to a
Raspberry Pi / Linux-class device later (DISPLAY_BRINGUP_PLAN.md) with only the bottom
layer changing.

---

## 1. Layering (top sits on bottom; only the bottom layer is platform-specific)

```
┌──────────────────────────────────────────────────────────────┐
│  Applications        Home · Clock · Calendar · Notifications · │  APP_MODEL.md
│  (built-in apps)     Camera · Gallery · Settings · AI Assistant│
├──────────────────────────────────────────────────────────────┤
│  Screen manager      stack · overlays · status strip · focus   │  §2
│  + Navigation model                                            │
├──────────────────────────────────────────────────────────────┤
│  Widget system       Label/Icon/List/Gauge/Toggle/… + layout   │  §3
├──────────────────────────────────────────────────────────────┤
│  Event system        input · timer · service · content events  │  §4
├──────────────────────────────────────────────────────────────┤
│  Input abstraction   raw HW → logical actions (configurable)   │  §5
├──────────────────────────────────────────────────────────────┤
│  Services            Notification · Settings · Clock · Battery  │  §6,§7  +
│  + local stores      · Calendar · Camera control  (device-owned)│  APP_MODEL.md
├──────────────────────────────────────────────────────────────┤
│  Rendering abstraction   Renderer (primitives) + Font provider  │  §8
├──────────────────────────────────────────────────────────────┤
│  Display HAL  sd_display  (packed framebuffer blit/present)     │  §8.3  ← PORT SEAM
└──────────────────────────────────────────────────────────────┘
                         ▲ everything above this line is identical
                           across ESP32-S3 and Raspberry Pi/Linux
```

**Design rule:** widgets and apps talk only to the Renderer (§8) and the Event/Screen
APIs. They never touch the display HAL or platform code. That single rule is what
makes the runtime portable.

---

## 2. Screen manager & navigation model

### 2.1 Screen
A `Screen` is one full-canvas surface owning a widget tree and a lifecycle:

```c
typedef struct sd_screen {
  const char *id;                          /* "home", "clock", "camera", … */
  void (*on_enter)(struct sd_screen*);     /* became visible / pushed */
  void (*on_leave)(struct sd_screen*);     /* hidden / popped */
  void (*on_resume)(struct sd_screen*);    /* overlay above it dismissed */
  void (*on_pause)(struct sd_screen*);     /* overlay opened above it */
  bool (*on_event)(struct sd_screen*, const sd_event_t*); /* true = consumed */
  sd_widget_t *root;                       /* widget tree to lay out + draw */
  void *state;                             /* app-private */
} sd_screen_t;
```

### 2.2 Two-layer composition
The visible image is composed of two layers, top-down:

1. **Navigation stack** — a LIFO of screens. The top is the *active* screen. `push`
   opens a screen, `pop`/BACK returns. The bottom is always **Home** (the launcher).
2. **Overlay layer** — modal or transient surfaces drawn *above* the stack without
   altering it: notification toast, low-battery warning, charging screen, AI-response
   panel. An overlay may be **modal** (captures input) or **passive** (display only,
   auto-dismiss). Overlays map directly to the orthogonal "overlay" states in
   UI_STATE_MACHINE.md.

A persistent **status strip** (clock, battery, link) is composited on every frame,
independent of both layers (DISPLAY_ARCHITECTURE.md §4).

### 2.3 Navigation model
- **Home is a launcher** (icon grid / carousel) and the root of the stack. From Home,
  SELECT on an app `push`es that app's screen.
- **BACK** pops one screen; at Home, BACK is a no-op (or sleeps the display).
- **HOME** (long-press BACK, configurable) clears the stack back to Home.
- **Within a screen**, NEXT/PREV move *focus* among focusable widgets (§3.4); SELECT
  activates the focused widget. So the same four logical actions (NEXT/PREV/SELECT/BACK,
  §5) drive both inter-screen and intra-screen navigation — essential for a 1-button
  device.
- **Overlays** can pre-empt: an arriving high-priority notification opens a passive
  overlay; the user can SELECT to open the Notifications app, or it auto-dismisses.

```c
void sd_nav_push(sd_screen_t*);   void sd_nav_pop(void);   void sd_nav_home(void);
void sd_overlay_show(sd_overlay_t*, bool modal);   void sd_overlay_dismiss(void);
sd_screen_t *sd_nav_active(void);
```

---

## 3. Widget system

Retained-tree widgets with dirty-rect invalidation: a tree is cheap to keep, and
redrawing only what changed is what keeps a tiny panel responsive and power-frugal.

### 3.1 Base widget

```c
typedef struct sd_widget {
  sd_rect_t  bounds;                 /* assigned by layout */
  bool       focusable, focused, dirty, hidden;
  void (*measure)(struct sd_widget*, sd_size_t avail, sd_size_t *out);
  void (*layout) (struct sd_widget*, sd_rect_t bounds);
  void (*draw)   (struct sd_widget*, sd_renderer_t*); /* draws within bounds */
  bool (*on_event)(struct sd_widget*, const sd_event_t*);
  struct sd_widget *parent, *first_child, *next_sibling;
  void *state;
} sd_widget_t;

void sd_widget_invalidate(sd_widget_t*);   /* mark dirty → scheduled for redraw */
```

### 3.2 Containers (layout)
`VBox`, `HBox`, `List` (scrollable, virtualized for long data), `Stack` (z-layered),
`Grid` (Home launcher). Containers run measure→layout on their children within their
bounds and honor the canvas `safe` margin (eyebox).

### 3.3 Leaf widgets (the V1 vocabulary)
`Label` (wrap + scroll + optional typewriter reveal), `Icon`, `Gauge`/`Bar`,
`BigNumber`, `ListRow`, `Banner`, `Arrow` (shape-coded for nav), `ImageView`
(Gallery / bitmap escape hatch), `Toggle`, `Slider`, `MenuItem`. Each is a pure
function of its state → pixels, so each is snapshot-testable on the host.

### 3.4 Focus model
A screen has at most one focused widget. NEXT/PREV walk the focusable widgets in tree
order (wrapping); the focused widget draws a focus ring; SELECT delivers an
`activate` event to it. This is the entire interaction grammar for a single button.

---

## 4. Event system

A small, synchronous, prioritized dispatch — decouples services from the UI so a
battery service or the phone-extension client never reaches into a screen directly.

### 4.1 Event taxonomy
```c
typedef enum {
  SD_EV_INPUT,         /* logical input action (§5): NEXT/PREV/SELECT/BACK/PRIMARY/… */
  SD_EV_TIMER,         /* a registered timer fired (clock tick, animation, timeout) */
  SD_EV_SYSTEM,        /* battery level/charge, link up/down, low-power, time changed */
  SD_EV_CONTENT,       /* phone-supplied content: ai_answer, nav_route, notify, sync  */
  SD_EV_APP,           /* app-internal / inter-app message */
} sd_event_kind_t;

typedef struct { sd_event_kind_t kind; uint32_t code; void *data; uint32_t ts; } sd_event_t;
```

### 4.2 Dispatch order (top-down, consumable)
`overlay (modal first) → active screen → global handlers`. A handler returning *true*
consumes the event. Unconsumed input falls through to global navigation (e.g. BACK).
`SD_EV_CONTENT`/`SD_EV_SYSTEM` are routed to subscribers (services + interested
screens), then may raise overlays (a notification, a low-battery warning).

### 4.3 The run loop (power-aware, single-threaded core)
```
forever:
  drain input driver        → SD_EV_INPUT
  advance timer wheel        → SD_EV_TIMER
  drain service/content queue→ SD_EV_SYSTEM / SD_EV_CONTENT
  dispatch all pending events (§4.2)
  if any widget dirty:
      layout dirty subtree → render dirty rects via Renderer (§8) → sd_display_blit
      sd_display_present()
  block until (next timer | input IRQ | service signal)   ← idle = no CPU, no draw
```
When nothing is dirty and no timer is near, the loop sleeps — the basis of the Idle
power state (UI_STATE_MACHINE.md). The core is single-threaded; services may run on
their own tasks and hand events in through a thread-safe queue.

---

## 5. Input abstraction

The UI never sees GPIO. An **input driver** translates raw hardware into **logical
actions**; a configurable **input map** (a device setting, §7) makes the hardware
swappable without touching screens.

```
raw HW                         logical action (what the UI consumes)
─────────────────────────────  ─────────────────────────────────────
1 button: short press        → SD_IN_SELECT
          long press         → SD_IN_BACK / HOME
          double press       → SD_IN_NEXT
(future) rotary encoder      → SD_IN_NEXT / SD_IN_PREV
(future) touch bar swipe     → SD_IN_NEXT / SD_IN_PREV / SELECT
(future) IMU nod / head turn → SD_IN_PRIMARY (ask) / dismiss
(future) phone as remote     → any logical action over the link
```

```c
typedef enum { SD_IN_NEXT, SD_IN_PREV, SD_IN_SELECT, SD_IN_BACK, SD_IN_HOME,
               SD_IN_PRIMARY /* capture / ask */ } sd_input_action_t;
```

Logical actions, not devices, are the contract. V1 ships the 1-button map; richer
input is a driver + a map entry, zero UI change. The phone, when present, can also
inject logical actions (it *extends* input; it doesn't own it).

---

## 6. Notification system

Device-owned: notifications are received, stored, viewed, and dismissed **on the
glasses**; the phone is one source among several (it relays app notifications), not
the owner.

```c
typedef struct {
  uint32_t id; const char *source;        /* "messages","system","alarm",… */
  const char *title, *body; uint16_t icon;
  uint32_t ts; uint8_t priority; bool read;
} sd_notification_t;
```

- **Store** — a persisted ring buffer in flash (survives reboot). Bounded; oldest
  evicted. Read/unread tracked.
- **Service** — ingests from (a) the phone-extension client (`notify` content message),
  and (b) **local sources** (low battery, charging, alarms, capture-saved) — proving
  notifications work with no phone. Emits `SD_EV_CONTENT(notify)`.
- **Presentation** — the runtime raises a **passive overlay** (auto-dismiss, priority-
  gated). The **Notifications app** (APP_MODEL.md) browses history.
- **Sync back** — dismiss/read emits `notify_dismissed`/`read` to the phone so the two
  stay consistent; if the phone is absent, the change is simply applied locally and
  reconciled on reconnect.

---

## 7. Settings framework

Schema-driven and **device-owned**. Settings are defined by descriptors; the Settings
app renders its UI generically from the registry, so adding a setting never means
writing a screen.

```c
typedef enum { SD_SET_BOOL, SD_SET_INT, SD_SET_ENUM, SD_SET_STRING } sd_set_type_t;
typedef struct {
  const char *key, *label, *group;        /* "display/brightness" */
  sd_set_type_t type;
  sd_value_t def, min, max;               /* range for INT; */
  const char **options; uint8_t n_options;/* for ENUM */
  void (*on_change)(const sd_value_t*);    /* apply live (e.g. brightness) */
} sd_setting_desc_t;

sd_err_t sd_settings_register(const sd_setting_desc_t*);
sd_err_t sd_settings_get(const char *key, sd_value_t *out);
sd_err_t sd_settings_set(const char *key, sd_value_t val);   /* persists + on_change */
```

- **Persistence**: NVS (the existing config store). The device is the source of truth.
- **Backup/restore**: the phone may store a copy (`settings_backup`) and restore it
  (`settings_restore`), but the glasses remain authoritative — restore is a *merge into*
  device state, never a takeover.
- **Examples**: `display/brightness`, `display/timeout`, `input/handedness`,
  `input/map`, `clock/24h`, `clock/tz`, `notify/priority_floor`, `power/idle_dim`.

---

## 8. Rendering abstraction (the portability seam)

Three sub-layers; only the lowest is platform-specific.

### 8.1 Renderer (platform-independent)
A draw context over an in-RAM canvas (the framebuffer). Widgets call only this.

```c
typedef struct sd_renderer sd_renderer_t;
void sd_r_pixel (sd_renderer_t*, int x, int y, sd_color_t);
void sd_r_hline (sd_renderer_t*, int x, int y, int w, sd_color_t);
void sd_r_rect  (sd_renderer_t*, sd_rect_t, sd_color_t, bool fill);
void sd_r_glyph (sd_renderer_t*, int x, int y, uint32_t cp, const sd_font_t*, sd_color_t);
void sd_r_text  (sd_renderer_t*, sd_rect_t, const char*, const sd_font_t*, sd_align_t, sd_color_t);
void sd_r_blit  (sd_renderer_t*, sd_rect_t, const uint8_t *packed, sd_pixfmt_t); /* images */
sd_rect_t sd_r_clip(sd_renderer_t*, sd_rect_t);   /* clip to dirty rect */
```

The default implementation is a **software rasterizer** writing into a packed
framebuffer in the device's native `sd_pixfmt_t` (mono1 for V1; gray4/rgb565 later).
A platform *may* supply an accelerated Renderer instead — widgets don't care.

### 8.2 Font provider
Bitmap fonts in flash with glyph metrics; pluggable (one compact mono font for V1, a
larger face + scalable fonts on Linux-class hardware). `sd_r_glyph`/`sd_r_text` go
through it. Glyph atlas in flash → cheap on the MCU.

### 8.3 Display HAL (`sd_display`) — the only platform-specific code
Packed-framebuffer blit/present, already defined in DISPLAY_ARCHITECTURE.md §7:

```c
sd_err_t sd_display_init(void);
sd_err_t sd_display_info(sd_display_info_t*);              /* w,h,format → DisplayProfile */
sd_err_t sd_display_blit(uint16_t x,uint16_t y,uint16_t w,uint16_t h,
                         const uint8_t *packed, size_t len);   /* one dirty rect */
sd_err_t sd_display_present(void);
const char *sd_display_backend(void);   /* "memfb" | "ssd1306" | "drm" | "sdl" | "stub" */
```

Backends: **`memfb`** (host, snapshot tests — the workhorse for pre-hardware dev),
**`ssd1306`/SPI-OLED** (ESP32 V1), **`drm`/`sdl`** (future Linux/RPi). Same runtime
above all of them. The default is an honest **stub** so a displayless build links and
the local services still run headless.

### 8.4 DisplayProfile (adaptation)
Layout reads a profile derived from `sd_display_info`: `{w,h,pixfmt,color,rotation,
safe_margin}`. Containers adapt to `w×h`; the status strip and safe margins scale.
This is how the *same* apps render on a 256×64 mono OLED today and a larger panel
later without per-screen changes.

---

## 9. Public runtime API (what apps & the phone client use)

Consistent with DISPLAY_ARCHITECTURE.md §7:

```c
sd_err_t sd_ui_init(void);                 /* renderer + display + screens + services */
void     sd_ui_tick(void);                 /* one iteration of the §4.3 loop */
void     sd_ui_input(sd_input_action_t);   /* feed a logical action (from §5 driver) */
void     sd_ui_post(const sd_event_t*);    /* services post system/content events */

/* content ingress from the phone-extension client (it extends; it never draws) */
void sd_ui_notify_push(const sd_notification_t*);
void sd_ui_ai_answer(uint32_t id, const char *text);
void sd_ui_nav_route(const sd_nav_route_t*);
void sd_ui_calendar_sync(const sd_event_t *events, size_t n);
```

---

## 10. Portability & test strategy

- **Portability**: everything above §8.3 compiles unchanged on any target; a new
  platform implements one `sd_display` backend (+ optionally an accelerated Renderer)
  and provides timers/NVS equivalents. DISPLAY_BRINGUP_PLAN.md is the recipe.
- **Host-first**: the `memfb` backend lets the entire runtime — apps, navigation,
  widgets, services, state machine — run and be **snapshot-tested on the host with no
  glasses**, exactly as the audio/camera loop was de-risked. Disconnect-behavior tests
  assert the device keeps rendering local state with the phone gone (the autonomy
  guarantee).
- **No firmware yet**: this document defines interfaces; implementation begins with the
  first item in DISPLAY_BRINGUP_PLAN.md's closing section.

---

## 11. Explicit non-goals (keep the runtime honest)

- No phone-rendered pixels for normal UI (bitmap blit is an escape hatch only).
- No on-device AI, route computation, or heavy processing (those legitimately exceed
  the MCU — the phone provides them as content).
- No multi-window compositor, no animation engine beyond simple tweens/dirty-rects.
- No dynamic app loading in V1 (built-in apps only; a plugin model is a V2+ topic).
