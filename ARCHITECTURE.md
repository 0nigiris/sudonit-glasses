# ARCHITECTURE.md

# System Architecture

This document describes the overall architecture of the Sudonit Smart Glasses platform.

---

# Governing Principle (canonical)

**Everything that can reasonably run on the glasses runs on the glasses. External
devices extend the system; they do not replace it.**

Sudonit is an autonomous device that becomes *more* powerful when connected to a
phone, server, or AI provider — never a thin client that is useless without one. The
glasses own and persist their own clock, calendar, settings, battery management,
notifications, camera control, and UI rendering, and remain functional with no phone
present. The phone/server *extends* the device with capabilities that genuinely exceed
an ESP32-class MCU (AI inference, navigation routing, cloud sync, large-scale
processing). This principle is canonical and overrides any older wording in this file.

See `docs/DISPLAY_ARCHITECTURE.md` §0 for the detailed model.

---

# High-Level Overview

+------------------+
| User |
+------------------+
|
v
+------------------+
| Smart Glasses |
+------------------+
|
v
+------------------+
| Smartphone |
+------------------+
|
v
+------------------+
| AI Provider |
+------------------+

The Smartphone and AI Provider tiers are **optional extensions**. The glasses are
the product; remove the lower tiers and the device still runs its local functions.

---

# Smart Glasses Responsibilities

The glasses are responsible for (and own the state for):

* Capturing images
* Playing audio
* Rendering the local UI / display
* Receiving user input and navigating local screens
* Clock and timekeeping
* Calendar / agenda (local store, synced)
* Settings (device-owned, persisted on device)
* Battery management
* Notifications (received, stored, viewed, dismissed on device)
* Camera controls
* Local storage of the above device-owned state
* Communicating with the smartphone (when present)

The glasses are NOT responsible for (these genuinely exceed the MCU and are the
phone's job to *provide*, not the glasses' to depend on):

* AI inference
* Navigation route computation
* Cloud communication
* Large-scale / heavy processing

---

# Smartphone Responsibilities

The smartphone is an **extension**, not the primary device. It provides capabilities
that exceed the glasses and enriches device-owned state by syncing — it is not the
source of truth for the device's own data.

Responsibilities:

* AI communication
* Navigation routing
* Networking / internet egress
* Cloud sync and backup (including a backup copy of device-owned settings)
* Large-scale processing and rich integrations
* Authentication
* Plugin management

The device owns its settings and local data; the phone may back them up and sync
them, but the glasses remain authoritative and functional without it.

---

# AI Provider Responsibilities

Examples:

* OpenAI
* Anthropic
* Local models

Responsibilities:

* Image analysis
* Language processing
* Reasoning
* Translation

---

# Communication Model

ESP32
↔
Smartphone

Communication methods:

Preferred:

* Bluetooth LE

Alternative:

* Wi-Fi Direct

Future:

* Hybrid Bluetooth + Wi-Fi

---

# Image Pipeline

User
↓
Capture
↓
Camera
↓
ESP32
↓
Phone
↓
AI
↓
Phone
↓
ESP32
↓
User

---

# Audio Pipeline

User Speech
↓
Microphone
↓
ESP32
↓
Phone
↓
AI
↓
Phone
↓
ESP32
↓
Speaker

---

# Display Pipeline

The phone sends **semantic content** (a notification payload, an AI answer, a route,
calendar events to sync) — not pixels. The glasses render their own UI locally from
that content and from local state. A raw-bitmap path exists only as an escape hatch
for content the device genuinely cannot render itself.

Local state / content
↓
(phone supplies content & sync — optional)
↓
ESP32 UI runtime (layout + widgets + fonts)
↓
Framebuffer
↓
Display
↓
User

The display also renders purely local screens (clock, settings, battery, calendar)
with no phone involved.

---

# Security Model

User Device
↔
Trusted Phone

Requirements:

* Device pairing
* Authentication
* Encryption
* Local trust model

---

# Future Architecture

Potential future variants:

* Raspberry Pi Edition
* Linux Edition
* Pro Edition

The software architecture should remain compatible whenever possible.

---

# Important Principle

Only what genuinely exceeds the device — AI inference, navigation routing, cloud, and
large-scale processing — belongs on the smartphone. Everything else runs on the
glasses.

The glasses should remain efficient and maintainable, but **autonomy comes before
minimal firmware**: we accept more on-device code in exchange for a device that stands
on its own. "Lightweight" means efficient, not dependent.


---

<!-- ===== consolidated from: docs/APP_MODEL.md ===== -->

# APP_MODEL.md

The built-in applications of the Sudonit on-device UI runtime
(UI_RUNTIME_ARCHITECTURE.md). Each app is a `sd_screen` (or a small set of screens)
plus a device-owned service and local store. **All of them run with no phone**; where
the phone helps, it *extends* with content/sync, it does not own the app.

Conventions used below:
- **Navigation** uses the four logical actions (NEXT/PREV/SELECT/BACK) plus PRIMARY
  (capture/ask) — the 1-button grammar of UI_RUNTIME_ARCHITECTURE.md §5.
- **Required data** lists what the app needs locally and, separately, what the phone
  may supply. If the phone column is empty, the app is fully autonomous.
- Apps are reached from **Home** (the launcher) and pushed onto the navigation stack.

---

## App registry (overview)

| App | Autonomous? | Local store | Phone extends with | Screen id |
|---|---|---|---|---|
| Home | ✅ fully | app list, layout | — | `home` |
| Clock | ✅ fully | RTC, alarms | time/tz sync | `clock` |
| Calendar | ✅ (after first sync) | event cache | calendar sync | `calendar` |
| Notifications | ✅ fully | notification ring buffer | relayed notifications | `notify` |
| Camera | ✅ fully | settings, captures | AI capture requests, upload | `camera` |
| Gallery | ✅ fully | captured media index | cloud backup/offload | `gallery` |
| Settings | ✅ fully | NVS settings registry | backup/restore | `settings` |
| AI Assistant | needs phone for answers | recent Q/A, transient | the AI answer itself | `ai` |

Navigation (the AI *answer overlay*, the notification overlay, low-battery, charging)
are **overlays/modes**, not launcher apps — see UI_STATE_MACHINE.md.

---

## 1. Home

**Purpose:** the launcher and default surface — the root of the navigation stack and
the screen shown on wake. Also the at-a-glance dashboard (clock + next event + unread
count + battery) so the most common information needs zero taps.

**Navigation flow:**
- Wake → Home. NEXT/PREV move focus across the app grid/carousel; SELECT pushes an app;
  BACK at Home does nothing (or sleeps). Long-press BACK = Home from anywhere.
- A glance widget row shows time, next calendar item, unread notifications, battery.

**Required data:** *local* — installed app list + order, and read-only snapshots from
the Clock/Calendar/Notifications/Battery services. *Phone* — none.

**Future expansion:** user-reorderable / hideable apps; complications (configurable
glance widgets); a "favorites" quick-launch on long-press; third-party app tiles when a
plugin model lands (V2+).

---

## 2. Clock

**Purpose:** timekeeping, the home-screen time source, alarms/timers. The most
fundamental proof of autonomy — a watch that needs no phone.

**Navigation flow:** Home → Clock. Faces cycle with NEXT (digital / analog / world).
SELECT opens alarms/timers; within, NEXT/PREV pick, SELECT toggles/edits, BACK exits.

**Required data:** *local* — the RTC (kept across deep sleep; an external RTC chip is a
hardware option for drift, see FRAME_ARCHITECTURE.md/BOARD_RESOURCES.md) and alarm
definitions in NVS. *Phone* — periodic `time_sync` (epoch + timezone) to correct drift
and handle DST; entirely optional, the clock runs without it.

**Future expansion:** stopwatch, multiple timezones, sunrise/sunset, calendar-aware
"leave by" alerts, haptic alarms when a vibration motor is added.

---

## 3. Calendar

**Purpose:** show today/upcoming events on the device, including the "next event" glance
on Home. Device-owned cache so the agenda is visible offline.

**Navigation flow:** Home → Calendar opens the agenda (next events as a `List`).
NEXT/PREV scroll, SELECT opens event detail (title, time, location, notes), BACK
returns. Day/agenda view toggle on a secondary action.

**Required data:** *local* — an event cache in flash (`{ts, end, title, loc, notes}`),
rendered with no phone once it has ever synced. *Phone* — `calendar_sync` pushes the
user's phone-calendar events; the device stores and renders them itself.

**Future expansion:** create/edit events on-device (sync back up), reminders/notifications
from events, week view on larger panels, multiple calendars with color/marker coding.

---

## 4. Notifications

**Purpose:** receive, store, view, and dismiss notifications **on the glasses**
(UI_RUNTIME_ARCHITECTURE.md §6). The history browser behind the transient overlays.

**Navigation flow:**
- Arrival → a passive **overlay** (auto-dismiss, priority-gated) over any screen.
- Home → Notifications opens the history `List` (unread emphasized). NEXT/PREV scroll,
  SELECT expands one (full title/body/time/source), BACK returns. SELECT-hold or a
  secondary action dismisses; a "clear all" lives at the list end.

**Required data:** *local* — the persisted notification ring buffer; **local sources**
(low battery, charging, alarms, capture-saved) populate it with no phone. *Phone* —
relays app notifications via the `notify` content message; dismiss/read sync back.

**Future expansion:** per-source filtering & mute, quick replies (phone-assisted),
grouping/threading, priority rules editable in Settings, do-not-disturb schedules.

---

## 5. Camera

**Purpose:** the device-owned capture surface — framing/preview, shutter, mode,
resolution — and the trigger for "look and ask." The glasses own the camera; the phone
may *request* a capture (for AI) or *receive* one (for upload), but control is local.

**Navigation flow:**
- Home → Camera enters **camera mode** (.UI_STATE_MACHINE.md): a live/last-frame
  preview with a minimal HUD (mode, resolution, battery). PRIMARY = capture. NEXT
  cycles mode (photo / ask-AI / burst-later), SELECT opens camera settings, BACK exits.
- "Ask-AI" mode: PRIMARY captures and hands the frame to the AI Assistant flow
  (capture stays a device action; the *answer* is the phone's extension).

**Required data:** *local* — camera HAL (OV5640 driver), capture/resolution settings in
NVS, and local storage for captures. *Phone* — `capture_request` (phone asks the device
to capture for AI), and optional upload/offload of captured media.

**Future expansion:** video clips, timelapse, exposure/white-balance controls, on-device
QR/barcode handling, a privacy capture indicator LED (the Glass lesson, COMPETITOR_ANALYSIS.md).

---

## 6. Gallery

**Purpose:** browse media captured by the Camera app — stored on-device, viewable
offline. The output side of camera autonomy.

**Navigation flow:** Home → Gallery shows a thumbnail `Grid`/`List` (most recent first).
NEXT/PREV move selection, SELECT opens full-frame view (`ImageView`), BACK returns. In
full view: NEXT/PREV step through items; a secondary action offers delete / "send to
phone".

**Required data:** *local* — a media index + the image files in flash/SD, decoded by the
`ImageView` widget / `sd_r_blit`. *Phone* — optional cloud backup/offload and freeing
on-device space; the gallery itself needs no phone.

**Future expansion:** albums/tags, on-device thumbnails cache, JPEG/streaming decode for
larger media, "share to AI" (re-ask about an old photo), storage management UI.

---

## 7. Settings

**Purpose:** view and change device-owned configuration. Renders **generically from the
settings registry** (UI_RUNTIME_ARCHITECTURE.md §7) — adding a setting never means
writing a screen.

**Navigation flow:** Home → Settings shows groups (Display, Input, Clock, Notifications,
Power, Connectivity, About). SELECT opens a group → a `List` of settings. For each:
Toggle flips on SELECT; Int/Slider adjusts with NEXT/PREV then SELECT to confirm; Enum
cycles options. BACK steps back out. Changes persist to NVS and call `on_change` live
(e.g. brightness).

**Required data:** *local* — the settings registry + NVS store; About reads firmware/
build/battery/storage. *Phone* — optional `settings_backup`/`restore` (device stays
authoritative).

**Future expansion:** Wi-Fi/pairing UI on-device, profiles, factory reset & diagnostics,
firmware-update entry point (OTA, later), accessibility settings (text size, high
contrast — mirrors UX Concept D).

---

## 8. AI Assistant

**Purpose:** ask about what you see/hear and get an answer. This is the one app whose
*intelligence* is genuinely beyond the device — so the **phone provides the answer**
while the glasses own the trigger, the capture, the rendering, and the history.

**Navigation flow:**
- Entered by PRIMARY (push-to-ask) from Camera/Home, or Home → AI Assistant.
- Flow: capture (device) → "thinking" indicator → answer rendered in an **AI-response
  overlay/mode** (UI_STATE_MACHINE.md), streamed in as it arrives; spoken via audio in
  parallel. BACK dismisses; SELECT may show "more / open on phone" for long answers.
- Recent Q/A is browsable as a short local `List`.

**Required data:** *local* — the captured frame, the "thinking"/answer UI, and a small
recent-history buffer. *Phone* — the AI call and the `ai_answer` content (and the PCM
audio downlink for the spoken reply, already implemented).

**Degradation:** with no phone, PRIMARY still *captures* (saved to Gallery) and the UI
explains the assistant is unavailable — the device fails gracefully, never blankly.

**Future expansion:** voice question (mic), conversational follow-ups, on-device wake
word (privacy-gated, later), proactive suggestions, translation/transcription surfaces
(UX Concepts C/D), pluggable providers (your key, your model — VISION.md).

---

## Cross-cutting notes

- **Every app degrades gracefully without the phone** (UI_STATE_MACHINE.md disconnect
  behavior). Only AI Assistant and phone-relayed notifications/calendar *content* pause;
  the apps themselves keep rendering local state.
- **Apps share the widget vocabulary and the status strip** — consistent look, small code.
- **New apps are additive**: register a screen + (optionally) a service/store; the
  launcher, navigation, settings, and rendering are reused unchanged. This is the payoff
  of the layered runtime and the reason the built-in set can grow toward the V2/V3
  feature list (PRODUCT_VISION.md) without architectural churn.


---

<!-- ===== consolidated from: docs/FRAME_ARCHITECTURE.md ===== -->

# FRAME_ARCHITECTURE.md

The physical architecture — where the parts go and why. Grounded in Sudonit's
defining constraint (VISION.md): **a clip-on module that attaches to glasses the user
already owns**, not a custom frame. That single decision drives weight, balance,
cable routing, and repairability more than any other.

V1 is a temple-mounted clip-on pod. V2/V3 may integrate into a designed frame, but
the placement logic below carries forward.

---

## 1. Form factor: the V1 clip-on pod

```
        side view of one temple arm of the user's existing glasses
        ════════════════════════════════════════════════  (lens)
   ear ─┐                                              hinge │
        │   ┌───────────── Sudonit pod ─────────────┐        │
        │   │  [battery]   [ESP32-S3 + PMIC]  [cam] │────────┘
        │   │      \________ flex/cable ________/   │
        │   └───────────────────────────────────────┘
        │        clip grips the temple arm
       (behind ear)                              (toward lens / eye)
```

The pod clamps to one temple arm. Mass concentrates **behind the hinge, toward the
ear**; sensors (camera, optional display optic) reach **forward, toward the lens**.
Everything heavy sits where the ear and head already bear weight.

---

## 2. Component placement (and the reasoning)

### Camera — front of the pod, near the lens/hinge, forward-facing
Must see what the wearer sees, so it lives at the **front edge, aligned with the line
of sight**, tilted to match gaze. As far forward as the clip allows without
overhanging the lens. The OV5640 + its short ribbon is the most placement-constrained
part; everything else accommodates it. A visible position also makes the camera
*honest* (a hardware capture LED, the Glass privacy lesson from COMPETITOR_ANALYSIS.md).

### Battery — rearmost, behind the ear
The heaviest single item → placed **furthest back, over/behind the ear**, to
counterbalance the forward camera/optics and keep the pod from tipping off the nose.
Behind-the-ear is also the coolest, least skin-contact-sensitive zone for a LiPo, and
the easiest to make **user-replaceable** (a slide-out cell, not glued) — a V2 promise
that starts as a V1 habit.

### Compute (ESP32-S3 + PMIC) — center of the pod
The S3 + charging/PMIC board sits **mid-pod**, between battery and front sensors:
shortest average trace/cable length to everything, and the warm part kept off the
skin and away from the battery. Antenna keep-out faces outward (away from the head)
for Wi-Fi/BLE.

### Speaker — underside, angled toward the ear canal
A small speaker near the **rear underside**, aimed at the ear, like Ray-Ban Meta's
open-ear approach. Private-ish, no earbud, no canal occlusion. Kept away from the mic
to limit feedback.

### Microphone (V2) — front underside, away from the speaker
Beam toward the mouth/forward, **physically separated from the speaker** (opposite
ends of the pod) to reduce echo/feedback. A second mic for noise rejection is a V2
nicety. (Mic is off the V1 critical path per the milestone scope.)

### Display optic (optional, V1; core, V2) — front, swung in front of the eye
The near-eye combiner (Option A beam-splitter in V1) mounts at the **front of the
pod and reaches inboard** to sit just in front of the eye/lens. It needs fine
position adjustment (eyebox is tiny), so it's on a **small adjustable arm**, not
fixed. This is the only part that intrudes into the wearer's view — deliberately
isolated so it can be omitted entirely (audio-only V1) without touching the rest.

---

## 3. Weight distribution

The enemy is **forward/nose-ward torque** — a pod that's front-heavy slides down the
nose and fatigues the user. Strategy:

- Battery (heaviest) **as far rear as possible** to counter the forward camera/optic.
- Target the pod's **center of mass behind the glasses' hinge**, over the ear pivot.
- Keep total added mass low: the lighter the pod, the less any imbalance matters. A
  clip-on is only acceptable if it's nearly unnoticeable in weight.
- **Symmetry note:** a one-side pod is inherently asymmetric. V1 accepts this (it's a
  prototype); V2 should consider a counterweight or splitting battery to the other
  temple via a thin over-the-back cable for balance.

---

## 4. Thermal considerations

Low power overall, but three sources matter: ESP32-S3 under Wi-Fi (bursty), the camera
during capture, and the audio amp during playback. Brownout (BOARD_RESOURCES.md §8) is
really a power *and* thermal question.

- **Separate the battery from the warm compute** (battery rear, S3 center) — heat and
  LiPos don't mix.
- **Keep warm surfaces off the skin**: the pod's head-facing wall is a thermal break;
  vent/conduct heat to the **outward** face.
- **Duty-cycle, don't sustain**: capture and Wi-Fi are bursts; the architecture's
  "phone does the heavy lifting" means the wearable rarely runs hot for long. Continuous
  video (a thing we *don't* do) is what would cook it — another reason the dumb-sensor
  design is also the cool-running design.
- Measure real current/temperature on the V1 pod before trusting any of this (it's on
  the day-one checklist already).

---

## 5. Cable routing

A clip-on has the camera ribbon and (optional) display/optic leads spanning a flexing,
user-handled object — the most likely mechanical failure point.

- **One short internal flex/ribbon spine** down the center of the pod connecting
  front sensors → center compute → rear battery. Minimize length; avoid loops.
- **Strain-relieve every connector**, especially the camera ribbon and any wire
  crossing the adjustable display arm (it moves → it fatigues).
- Route data away from the antenna keep-out and from the noisy I2S/camera-clock lines
  where possible (the AUDIO_BRINGUP buzz risk).
- **No cable crosses the clip joint** if avoidable — the clip is handled constantly;
  wires there break. Keep the clip purely mechanical.

---

## 6. Future repairability (a V1 habit, a V2 promise)

Repairability is a core value (VISION.md), and it's cheapest to honor by *designing
for it from V1* rather than retrofitting:

- **Modules, not a monolith** — the FRAME maps to three separable units: **camera
  front-end**, **compute+power core**, **display optic**. Each can be replaced or
  omitted independently (this is also why audio-only V1 is trivial).
- **Battery is a service part** — slide-out or connectorized, never glued. The single
  most-failing component must be the easiest to swap.
- **Connectors over solder** at module boundaries (camera ribbon, display lead, battery)
  so a field repair is a reseat, not a rework station.
- **Fasteners, not adhesive**, for the enclosure — openable with a common tool.
- **The clip is a separate, replaceable part** sized per glasses style — it wears and
  it's the cheapest thing to reprint/remake.

These choices cost a little size/weight in V1; they buy the entire repairability and
modularity story the product is sold on. The tradeoff is deliberate and worth it.

---

## 7. Open questions for the physical V1 (to resolve with the board in hand)

- Real pod mass and balance once the OV5640 board + LiPo + speaker are placed — does
  it stay on the nose? (Counterweight needed?)
- Camera ribbon length/orientation off the ordered S3-CAM board vs the forward mount.
- Whether GPIO 40/41/42 + speaker physically fit the rear-underside speaker placement
  without fouling the clip (ties to BOARD_RESOURCES audio breakout verify).
- Display arm adjustability range needed to hit the eyebox for different faces.
- Bracket print: temple-arm clamp dimensions vary wildly by glasses — V1 needs one
  reference pair to design against.

These are physical-prototype tasks, not architecture gaps; the placement logic above
holds regardless of how they resolve.
