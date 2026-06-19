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
