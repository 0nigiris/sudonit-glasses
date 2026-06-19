# UI_STATE_MACHINE.md

The complete state machine for the Sudonit device UI. It governs the runtime in
UI_RUNTIME_ARCHITECTURE.md and the apps in APP_MODEL.md. The whole model exists to
guarantee the autonomy promise: **there is no state in which the device becomes a
useless black screen because the phone is away** (DISPLAY_ARCHITECTURE.md §8).

The requested states mix two *independent* concerns — device power/lifecycle, and what
the user is looking at. Modeling them as one flat list would force false transitions
(e.g. "you can't be charging and in the camera"). So the machine is a **statechart with
three orthogonal regions** that run concurrently:

```
  ┌────────────────────────────────────────────────────────────────┐
  │ REGION P — Power / lifecycle   Boot → Active ⇄ Idle ; LowBatt ; │
  │                                Charging ; Shutdown               │
  ├────────────────────────────────────────────────────────────────┤
  │ REGION U — UI mode (only meaningful while Active)                │
  │   Home ⇄ App(Clock/Calendar/Notifications/Camera/Gallery/        │
  │            Settings) ; CameraMode ; AIResponse                   │
  ├────────────────────────────────────────────────────────────────┤
  │ REGION O — Overlays (compose above U, may pre-empt input)        │
  │   none | NotificationOverlay | LowBatteryWarning | ChargingScreen│
  └────────────────────────────────────────────────────────────────┘
  REGION L — Link (Connected/Disconnected) is tracked but only changes
             *content availability*, never blocks a UI state (autonomy).
```

The eight to nine names in the brief map onto these regions: *boot, idle, low battery,
charging* are **P**; *active, settings, camera mode, AI response* are **U**;
*notification overlay* is **O** (and low-battery/charging also surface as **O**).

---

## Region P — Power / lifecycle

```
        power-on
           │
           ▼
       ┌────────┐  init ok        ┌──────────┐  inactivity T1   ┌──────────┐
       │  BOOT  │ ───────────────►│  ACTIVE  │ ───────────────► │  IDLE    │
       └────────┘                 │ (awake)  │ ◄─────────────── │ (dim/    │
           │ init fail             └──────────┘  any input/event │  sleep)  │
           ▼                          ▲   │                      └──────────┘
       ┌────────┐                     │   │ battery < crit            │
       │ FAULT  │   (safe screen,     │   ▼                           │
       └────────┘    log, retry)   ┌───────────┐                      │
                                   │ LOW_BATT  │ ◄────────────────────┘
   charger in ──────────────┐      └───────────┘   (warn, then force Idle/Shutdown)
                            ▼            │ battery critical
                      ┌───────────┐      ▼
                      │ CHARGING  │  ┌──────────┐
                      └───────────┘  │ SHUTDOWN │
                                     └──────────┘
```

| State | Entry actions | Exit / transitions |
|---|---|---|
| **BOOT** | init HAL → display → renderer → services; show splash/boot screen; restore settings + RTC; load notification/calendar/gallery stores | init ok → **ACTIVE**; fatal init error → **FAULT** (honest error screen, never a blank panel) |
| **ACTIVE (awake)** | full brightness; Region U runs; status strip live | inactivity timer `T1` → **IDLE**; `battery<crit` → **LOW_BATT**; charger attached → raise **ChargingScreen** overlay (stays Active) ; power-off → **SHUTDOWN** |
| **IDLE (dim/sleep)** | dim then blank panel; suspend the render loop; keep RTC + wake sources; (deep sleep tiers as power allows) | any input / high-priority event / charger → **ACTIVE** (wake to last screen or Home per setting) |
| **LOW_BATT** | raise **LowBatteryWarning** overlay; reduce brightness; disable non-essential services (camera) | charging → **CHARGING**; `battery<shutdown` → **SHUTDOWN**; recovered → **ACTIVE** |
| **CHARGING** | show **ChargingScreen** (level/animation); allow use while charging | charger removed → **ACTIVE**/**IDLE** as before; full → notify |
| **SHUTDOWN** | persist all stores + settings; flush; power down gracefully | power-on → **BOOT** |
| **FAULT** | display a readable fault screen + code; log; offer retry/safe-mode | retry → **BOOT**; unrecoverable → safe halt |

`T1` (idle timeout) and dim/sleep tiers are device settings (Settings → Power).

---

## Region U — UI mode (active only)

This is the navigation stack of UI_RUNTIME_ARCHITECTURE.md §2, expressed as states.

```
   ┌────────┐  SELECT(app)   ┌──────────────────────────────┐
   │  HOME  │ ─────────────► │  APP  (Clock/Calendar/Notif/  │
   │(launcher)◄───── BACK ── │       Gallery/Settings)       │
   └────────┘                └──────────────────────────────┘
       │ SELECT(Camera)              │ open Camera
       ▼                             ▼
   ┌──────────────┐  PRIMARY   ┌───────────────────┐  answer arrives
   │ CAMERA_MODE  │ ─────────► │  (capture handed   │ ───────────────┐
   │ (preview/HUD)│            │   to AI flow)      │                 ▼
   └──────────────┘            └───────────────────┘        ┌──────────────┐
       ▲  BACK                                              │ AI_RESPONSE  │
       └──────────────────────── BACK / dismiss ────────────│ (thinking →  │
                                                            │  answer)     │
                                                            └──────────────┘
```

| State | Meaning | Key transitions |
|---|---|---|
| **HOME** | launcher + glance dashboard (APP_MODEL §1) | SELECT app → **APP**/**CAMERA_MODE**; PRIMARY → **AI_RESPONSE** (ask) |
| **APP** | a pushed app screen (Clock, Calendar, Notifications, Gallery, **Settings**) | BACK → pop (→ HOME or parent); deep screens are nested pushes |
| **CAMERA_MODE** | live/last-frame preview + capture HUD (APP_MODEL §5) | PRIMARY = capture; in ask-mode capture → **AI_RESPONSE**; BACK → HOME |
| **AI_RESPONSE** | "thinking" then streamed answer (overlay-style mode; audio in parallel) | answer done/BACK → return to previous U state; if no phone → graceful "unavailable" + save capture |

*Settings* is simply an **APP** state; *camera mode* and *AI response* get their own
states because they have distinct input grammars (PRIMARY=capture; streaming answer).

---

## Region O — Overlays

Overlays compose **above** Region U and may capture input. They correspond to the
overlay layer in UI_RUNTIME_ARCHITECTURE.md §2.2 and are raised by Region P / services.

| Overlay | Raised by | Modal? | Dismiss |
|---|---|---|---|
| **NotificationOverlay** | notification arrives (phone or local source) | passive | auto-timeout; or SELECT → open Notifications app; BACK |
| **LowBatteryWarning** | Region P enters LOW_BATT / crosses threshold | passive→sticky | acknowledge; clears on charge |
| **ChargingScreen** | charger attached (Region P CHARGING) | passive | charger removed; or BACK to keep using |

Overlays never destroy the underlying U state — when dismissed, the user is exactly
where they were. Multiple overlays resolve by priority (low-battery > charging >
notification).

---

## Region L — Link (content availability, not a UI gate)

| Link state | Effect |
|---|---|
| **Connected** | AI answers, phone-relayed notifications, calendar sync, time sync flow in (content events) |
| **Disconnected** | those *content sources* pause; **every UI state and every local app keeps working**: clock ticks, calendar shows last sync, notifications history + local sources, camera/gallery/settings fully usable, current nav route keeps counting down |

Link transitions raise at most a small status-strip indicator and (optionally) a low-
priority notification — they **never** force a UI state change. This row is the
formal statement of the autonomy guarantee.

---

## Worked transition examples

1. **Cold start, no phone ever paired:** BOOT → ACTIVE → HOME. User opens Clock,
   sets an alarm, browses Gallery — all in Region U with Region L = Disconnected. Fully
   functional.
2. **Notification while in Settings:** U=APP(Settings); a `notify` event raises O=
   NotificationOverlay above it; auto-dismiss → back to Settings exactly where left.
3. **Ask while walking:** U=HOME, PRIMARY → capture (device) → U=AI_RESPONSE shows
   "thinking"; phone returns `ai_answer` → streamed text + spoken audio; BACK → HOME.
4. **Battery drains mid-camera:** U=CAMERA_MODE; P crosses crit → LOW_BATT raises
   O=LowBatteryWarning, disables camera; user plugs in → P=CHARGING, O=ChargingScreen;
   continues. No data lost.
5. **Phone dies mid-navigation:** L→Disconnected; the nav route already on the device
   keeps counting down from local timers; only *reroutes* are unavailable until L returns.

---

## Persistence & safety

- **Every store (settings, notifications, calendar, gallery index, clock/alarms)** is
  persisted on change and flushed on SHUTDOWN, so BOOT always restores a coherent device.
- **FAULT is a first-class state**: an init or runtime failure shows a readable screen
  with a code, never a blank/garbage panel — autonomy includes failing visibly.
- Region P guards (LOW_BATT, SHUTDOWN thresholds) protect the battery regardless of
  what Region U/O are doing.

---

## Host-testability

Because the runtime renders into the `memfb` backend, this entire statechart is
**executable and snapshot-testable on the host with no hardware**: drive synthetic
input/timer/system/content events, assert the resulting state + framebuffer. The
disconnect rows (Region L) are tested explicitly — feed a link-down event and assert
every U state still renders from local stores. That test *is* the autonomy guarantee,
checked in CI before any panel exists.
