# MVP_DEFINITION.md

> **The one question:** *What is the smallest possible version of Sudonit that still
> feels magical?*

**The answer:** *A device on your glasses that is alive on its own — it knows the
time, holds its settings, can capture — and when you press a button and look at
something, a voice tells you about it, because the phone extends it with AI.*

The magic has two halves, and the order matters under our governing principle
(DISPLAY_ARCHITECTURE.md §0): **the glasses are an autonomous device first, and the
phone extends them.** So the MVP is *not* "a button that pings a phone." It is a small
standalone wearable (local clock/UI/settings/capture that works with the phone off)
whose headline trick — **"my glasses can see and answer"** — is delivered by the phone
*adding* AI on top. Camera + button + the autonomous baseline live on the device; the
AI is the extension.

What the magic still does **not** need: *no microphone, no app store, no waveguide, no
on-device AI.* Those make it better; they don't make it magic. But it **does** need the
device to stand on its own — a glasses display that goes black without a phone is not a
product, it's a peripheral.

This is not a guess. **Ray-Ban Meta's base model has no display and sells well**
(COMPETITOR_ANALYSIS.md). The "look and ask, hear the answer" loop is independently
proven to feel magical. Sudonit's job for the MVP is to reproduce *that* loop, openly
and cheaply — and we have already proven the entire software half of it on the host.

---

## Must-have

### A. Autonomous baseline (the device is alive without a phone)

1. **On-device UI runtime** — local rendering of the device's own screens
   (DISPLAY_ARCHITECTURE.md §3). *Minimal* for the MVP, but real: the glasses draw
   their own pixels, they are not a remote framebuffer. *(New firmware — embraced, not
   minimized.)*
2. **Clock / home screen** — local timekeeping, shown with no phone present.
3. **Local settings** — a few device-owned options persisted in NVS. *(Config store
   exists.)*
4. **Basic battery state** — a local readout; full management is staged, but the device
   *owns* it, never the phone.
5. **Camera capture on demand** — see what the wearer sees. *(OV5640 driver written,
   pending bring-up.)*

> If the display track lags, this baseline runs headless (audio + capture) — but the
> local services still run on the device. The autonomy is architectural, not optional.

### B. The AI extension (the magic the phone adds on top)

6. **Glasses → phone link** — carry capture up, content down. *(Wi-Fi transport
   implemented.)*
7. **Phone → AI → answer** — the intelligence the phone *provides*. *(Pipeline proven
   on host; real-Claude run is the one blocked step, A1.)*
8. **Spoken answer in the ear** — closes the loop. *(MAX98357A driver written + PCM
   downlink proven end-to-end on host.)*
9. **A trigger** — one physical button for navigation + push-to-ask. *(GPIO input.)*
10. **A way to wear it** — a clip-on bracket onto existing glasses. *(FRAME_ARCHITECTURE
    V1 pod.)*
11. **Provisioning + your own AI key** — Wi-Fi creds + key without hardcoding secrets.
    *(Serial provisioning + config store done.)*

The B-half loop is proven on host and gated on hardware bring-up. The A-half is the
**new work this philosophy adds**: a small on-device UI runtime + clock/settings/
battery services. That is deliberate — we trade more firmware for a device that is
independent, exactly as the architecture now requires.

---

## Nice-to-have (makes it better, not magic — add after the loop works)

- **Glance display fitted** (Option A monochrome OLED) — renders the device's own UI
  (clock, status, answer echo) locally. High-value; the MVP's *runtime* exists either
  way, this just gives it a panel. Parallel track if optics lag.
- **Calendar / agenda screen + sync** — a device-owned screen the phone enriches. The
  next local service after clock/settings.
- **Notifications screen** — view phone-relayed notifications on-device.
- **Navigation screen** — device renders the arrow locally from a phone-supplied route.
- **Microphone / voice trigger** — ask by talking instead of pressing. The button works.
- **Untethered battery life** — run off a LiPo for a day. MVP can run off USB for the
  first demo (but battery *state* is already device-owned, see must-have A4).
- **Real phone app** — beyond today's Python server. The server is enough to prove it.

---

## Explicitly postponed (do **not** build for the MVP)

Per the milestone's critical rule and the project's anti-complexity stance:

- **BLE control plane** — Wi-Fi is enough for the first demo.
- **Full battery management / fuel-gauge IC** — a *basic* device-owned readout is in
  the MVP (must-have A4); the rich charge-curve/health story is staged. USB for the demo.
- **Binocular / full-color / waveguide display** — V2/V3, supply-gated.
- **Wake-word / always-listening** — privacy + power cost, not needed to feel magic.
- **OTA updates, multi-device, accounts, cloud** — pure infrastructure, zero magic.
- **On-device AI inference / route computation** — these legitimately exceed an ESP32;
  the phone *provides* them (the Humane Pin lesson is about putting the *AI* on the
  wearable, not about putting the *UI* there). **Note the change from an earlier draft:
  on-device *rendering* is now REQUIRED, not postponed** — it is the autonomous baseline
  (must-have A1). Only on-device *intelligence* stays off-device.
- **Polished enclosure / industrial design** — the MVP is tape-and-bracket honest.

If a task isn't on the must-have list, the default answer for the MVP is **no**.

---

## Fastest path to the magic (critical path only)

Two tracks. The **autonomy track** can be built *now*, on the host, with no hardware
(the UI runtime + local services run against the memory-framebuffer backend). The
**extension track** is the bring-up sequence already proven on host.

*Autonomy track (host, in transit):*
- **A.** Build the minimal on-device UI runtime + clock/settings/battery services and
  snapshot-test them headless (DISPLAY_ARCHITECTURE §3–4, §10). This is the new work
  the philosophy adds and it needs no board.

*Extension track (when the board lands):*
1. **Bring up the board** — flash, confirm PSRAM, confirm camera pin map.
   *(BOARD_RESOURCES §9 steps 1–3.)*
2. **Camera → phone → AI on real hardware** — the success criterion. *(steps 4–6.)*
3. **Run real Claude (A1)** — supply the API key; close the one blocked software step.
4. **Audio out on hardware** — wire the MAX98357A, hear the answer. *(step 7.)*
5. **Add the button + clip it to glasses**, and bind the button into the UI runtime so
   it both navigates local screens and triggers push-to-ask.

When step 5 meets track A, the magic exists on a device that *also stands on its own*:
the glasses show the time and hold their settings with the phone off, and *press, look,
hear* works because the phone extends them. Everything in "nice-to-have"/"postponed" is
improvement on top. **Optimize for reaching that join; defend it against every tempting
addition — but never by hollowing out the device's autonomy to save firmware.**
