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
