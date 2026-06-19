# PRODUCT_VISION.md

How Sudonit evolves from a breadboard prototype into a product a small team can
ship — three versions, each a *complete, useful thing on its own*, not a milestone
toward a distant dream. The throughline (VISION.md) holds across all three: **open,
modular, repairable, privacy-respecting, and an autonomous device first.**

**Governing principle:** *everything that can reasonably run on the glasses runs on
the glasses; external devices extend the system, they do not replace it*
(DISPLAY_ARCHITECTURE.md §0). Sudonit is a **standalone wearable that becomes more
powerful when connected** to a phone, server, or AI provider — never a thin client
that is useless without one. The phone provides AI, navigation routing, cloud sync,
and large-scale processing; the glasses own their own clock, calendar, settings,
battery management, notifications, camera control, and UI rendering. We optimize for
**device independence over minimal firmware.**

Stated bluntly so the roadmap stays honest: **V1 is for us, V2 is for enthusiasts,
V3 is for normal people.** Each version must feel finished to *its* audience before
the next starts.

---

## Sudonit V1 — "The honest prototype"

**Target user:** the builder (us) and a handful of hardware-comfortable early
adopters who will tolerate wires, tape, and a clip-on lump.

**Main features:**
- **Autonomous baseline** — even with no phone: an on-device clock/home screen,
  local settings, basic battery readout, camera capture to local storage, and the
  on-glasses UI runtime that renders them (DISPLAY_ARCHITECTURE.md §3–4).
- Camera capture → (phone) AI → spoken answer — the phone *extends* the device by
  supplying the AI; the device owns the capture, the trigger, and the rendering.
- Audio out via the bonded speaker (MAX98357A).
- Wi-Fi data plane to the phone; serial provisioning.
- **Optional** monocular monochrome glance display (Option A OLED) rendering the
  device's *own* UI locally — a parallel track, *not* a ship-blocker. Audio-only is a
  valid V1 (the local services still run, headless).
- One physical button for navigation + push-to-ask.

**Hardware requirements:** ESP32-S3 N16R8 CAM + OV5640 (in hand), MAX98357A + small
speaker, optional SPI OLED + beam-splitter, USB power or a small LiPo, 3D-printed or
hand-built clip-on bracket onto existing glasses.

**Estimated cost (BOM, qty 1):** **$25–$60** depending on whether the display is
fitted. This is the whole point — the cheap BOM comes from the phone doing the AI.

**Technical risks:** brownout under camera+Wi-Fi(+audio) load; near-eye optics for
the OLED being focusable; clip-on ergonomics; battery life if untethered. All are
*bring-up* risks, not architecture risks — the software loop is already proven on host.

**Definition of done:** a person wearing existing glasses with the clip-on can press a
button, have the camera see what they see, and hear an AI answer — and, if the display
track lands, glance a one-line version of it. See MVP_DEFINITION.md.

---

## Sudonit V2 — "The enthusiast product"

**Target user:** open-hardware enthusiasts, accessibility tinkerers, and developers
who want a *hackable* Ray-Ban-Meta alternative they actually own.

**Main features:**
- Everything in V1, but **integrated**: no loose wires, a real enclosure, all-day-ish
  battery, BLE control plane so it pairs like a normal accessory.
- A **wearable monocular display** worth keeping on: a bought microLED/waveguide or
  well-documented HUD engine (DISPLAY_EVALUATION.md V2 stack) showing notifications,
  AI answers, navigation, and live transcription.
- Microphone in (push-to-talk and wake-word handled on the phone).
- A companion phone app (beyond today's Python server) with the user's own API key —
  privacy by construction.
- Modular/repairable: replaceable battery, separable compute/optics/camera modules.

**Hardware requirements:** a custom or semi-custom PCB carrier for the S3, integrated
battery + charging, the chosen display engine, mic, a designed frame/clip (FRAME_
ARCHITECTURE.md). Still ESP32-S3 class — no SoC change needed.

**Estimated cost (BOM):** **$80–$180**, retail target well under the closed
competition because there is no on-board AI silicon and no proprietary cloud.

**Technical risks:** sourcing the display engine in small quantity (supply-gated, not
technical); thermal and battery in a real enclosure; BLE throughput for the display
stream; manufacturing a wearable frame as a small team.

**Why it's a product, not a prototype:** it survives a normal day, looks acceptable in
public, and a non-builder enthusiast can set it up. It directly contests Ray-Ban Meta
on *openness and repairability*, not on pixel count.

---

## Sudonit V3 — "The normal-person product"

**Target user:** ordinary people who want subtle, useful glasses and have never heard
the word "ESP32."

**Main features:**
- Looks like ordinary glasses (or a barely-there clip), all-day battery, durable.
- The display "disappears": bright enough for daylight, invisible to onlookers,
  glanceable. Possibly binocular if the optics/research justify it (a V3 *bet*, not a
  promise — DISPLAY_EVALUATION.md keeps monocular as the safe default).
- A polished assistant experience: proactive notifications, navigation, translation/
  transcription, visual lookup — all still phone-brokered and privacy-respecting.
- An ecosystem: third-party "screens"/apps on the open platform.

**Hardware requirements:** custom optics + frame industrial design, custom or
co-designed compute (could still be ESP32-class or a step up if the display demands
it), real supply-chain partners for the display engine. This is where "one person"
becomes "a small team + partners."

**Estimated cost (BOM):** **$150–$350**; retail competitive with mid-range smart
glasses while remaining open.

**Technical risks:** these are *business and supply-chain* risks more than code:
display-engine sourcing at volume, optical manufacturing, certification, industrial
design, sustaining an open ecosystem. The compute/software architecture set in V1
should still carry through.

---

## The evolution, in one line each

- **V1** proves an *autonomous wearable* (local clock/UI/camera/settings) that the
  phone *extends* with AI — cheaply and openly.
- **V2** turns it into a *thing you can wear all day, own, and use standalone*.
- **V3** makes it *disappear* into normal glasses for normal people.

The constant across all three — **an autonomous device, extended (not replaced) by
the phone; your data; open and repairable** — is the moat. Nothing in the V1
architecture has to be thrown away to reach V3; the display and frame get better, and
the local runtime grows richer, while the autonomy model and privacy stance do not
change. The phone always *adds* capability (AI, navigation, cloud) on top of a device
that already works on its own. That independence is the deliberate payoff: a Sudonit
that keeps working when the phone is dead, absent, or someday unsupported.
