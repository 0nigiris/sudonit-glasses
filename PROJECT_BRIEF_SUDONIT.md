# PROJECT_BRIEF_SUDONIT.md

> **The single source of truth for anyone describing Sudonit to the outside world.**
> Website builders, designers, marketers, new contributors, and AI agents should be
> able to read *only this file* and understand the product completely. It is written
> for humans, not engineers. Where older documents in this repository disagree with
> what is written here, **this file and the founder's vision win.**

---

## 1. What is Sudonit?

**Sudonit is a small smart-glasses module that clips onto the glasses you already own.**

It is not a new pair of glasses. It is a lightweight pod — camera, speaker, Wi‑Fi, a
small computer and a button — that attaches to the temple arm of your existing frames
and turns them into smart glasses. When you want to know about something in front of
you, you press the button, the camera sees what you see, and a voice in your ear tells
you the answer.

**The problem it solves.** Today's smart glasses ask you to throw away the glasses you
love and buy *their* frames — a closed, sealed, take-it-or-leave-it gadget you don't
really own and can't repair, modify, or upgrade. Most people don't want a second pair
of glasses; they want *their* glasses to do more. Sudonit lets you keep your frames and
add intelligence to them.

**Why it exists.** Because smart glasses should be **owned by the person wearing them** —
open, repairable, modular, and not locked to one company's cloud, one company's frames,
or one company's AI. (See §10 for the human version of this.)

**Who it is for.**
- **Today (V1):** makers, hardware-comfortable early adopters, accessibility tinkerers,
  and developers who want an open device they actually control.
- **Tomorrow (V2):** open-hardware enthusiasts who want a hackable, repairable
  alternative to closed smart glasses.
- **Eventually (V3):** ordinary people who want subtle, useful glasses and have never
  heard the word "ESP32."

**What makes it different.** Three things, in one sentence: **you keep your own glasses,
you own and can repair the hardware, and you choose who runs the AI.** No proprietary
frame, no sealed box, no forced subscription, no single vendor.

---

## 2. Founder Vision

Sudonit's guiding analogy is **the ThinkPad, applied to smart glasses** — not the polished,
sealed consumer gadget, but the tool that is *repairable, modifiable, owner-controlled,
open, and modular.* The person wearing it owns it, can open it, can fix it, and can change
it. The core beliefs:

- **Autonomy-first.** The glasses are a standalone device. The phone, the cloud, and the
  AI provider *extend* the system — they never *define* it. The device must stay useful
  with no phone present (it can tell the time, hold its settings, and capture).
- **User ownership.** You buy the hardware and it is yours. It keeps working after purchase,
  with no mandatory subscription.
- **Repairability.** Components are meant to be replaced, not glued shut. The battery is a
  service part; modules separate; the clip is a cheap, reprintable piece.
- **Modularity.** Different batteries, microphones, audio solutions, displays, and future
  upgrades. Replaceable components are preferred over sealed integration. Modularity is a
  *feature*, not a compromise.
- **Privacy-friendly (not cloud-free).** The user decides where their data and their AI
  live. Cloud AI is *allowed* — because AI inference is one of the few things that
  genuinely exceeds a tiny wearable — but it is never forced onto a single provider, and
  your data belongs to you.
- **Open ecosystem.** Open firmware, open documentation, modifiable software, and room for
  community-made and official accessories.

**Sudonit vs. Ray-Ban Meta — the honest difference.**
Sudonit is **not** trying to out-polish Ray-Ban Meta or build a closed consumer ecosystem.
Ray-Ban Meta is a beautiful sealed product: their frames, their cloud, their rules, not
repairable, not yours to modify. Sudonit competes on a *different axis entirely* —
**openness, ownership, repairability, and the freedom to use your own glasses and your own
AI.** If Ray-Ban Meta is the iPhone of smart glasses, Sudonit is the ThinkPad: less shiny
on day one, but open, fixable, upgradable, and genuinely yours.

---

## 3. Product Concept

**Sudonit is a MODULE.** This is the heart of the product and the thing to lead with.

- **It clips onto existing glasses.** The pod clamps to one temple arm of the frames you
  already wear. There is **no proprietary Sudonit frame** to buy — that is a deliberate
  non-goal.
- **You can move the module between glasses.** Sunglasses today, reading glasses tomorrow,
  your nice frames for the evening — the same module moves with you. This portability is a
  core differentiator, not an afterthought.
- **Balanced weight distribution.** The pod is designed so the heaviest part (the battery)
  sits **behind the ear**, counterbalancing the camera at the front, so it doesn't drag on
  your nose. The goal is a module light enough to forget you're wearing.
- **All hardware stays on the head.** Camera, computer, battery, and speaker all live in
  the pod on the glasses.
- **No battery pack in your pocket.** Sudonit is self-contained on the frames — there is no
  wire running down to a separate box. (The phone, when used, only adds AI; it is not a
  power tether and not required for the device to be alive.)

The look matters too: the long-term aesthetic goal is **"if you're not looking closely, you
shouldn't notice the technology."** Normal-looking glasses, not cyberpunk.

---

## 4. Product Principles

- **The user owns the hardware.** Buy it once; it's yours.
- **The user can replace components.** Battery, and over time microphones, audio, displays,
  and the clip itself.
- **The user can use their own AI.** Bring your own API key.
- **The user can use hosted AI.** A convenient Sudonit-hosted option for people who don't
  want to manage a key (planned as an optional paid service).
- **The user can use their own server.** Self-hosted / local AI is a supported direction.
- **The AI is provider-agnostic.** Sudonit never forces one AI vendor; providers are
  replaceable.
- **The phone extends the device.** It supplies AI and heavy lifting; it does not replace
  the device or own its basic functions.
- **The cloud is optional.** It adds capability; it is never a gate to basic use.
- **Functionality stays local whenever practical.** Clock, settings, capture, and the
  device's own controls run on the glasses; only the genuinely-too-heavy work (AI
  inference) goes off-device.
- **No mandatory subscriptions.** The hardware must keep working without paying anyone.
- **Privacy by the user's choice.** Your data is yours; you decide where it goes.

---

## 5. Current V1 Product — "the honest prototype"

V1 is **audio-first**: you look, you ask, you *hear* the answer. There is no screen required.

**The core experience (camera → AI → audio):**
1. You press the button on the pod.
2. The camera captures what's in front of you.
3. The image travels over Wi‑Fi to the phone, which forwards it to an AI model.
4. The AI describes or answers.
5. The answer is spoken back through the pod's small speaker, into your ear.

And, with the phone away, the device is still *alive* on its own: it knows the time, holds
its settings, and can capture — it is a standalone wearable that the phone makes smarter,
not a dead peripheral.

**Hardware that exists today:** an ESP32‑S3 microcontroller (the small computer), an OV5640
camera, a MAX98357A speaker/amplifier, Wi‑Fi, one physical button, and a 3D‑printed /
hand-built clip-on bracket onto existing glasses. Power from USB or a small battery. The
estimated parts cost is roughly **$25–$60** — deliberately cheap, because the phone does the
expensive AI rather than costly on-board AI chips.

**Features included in V1:**
- Look-and-ask, hear-the-answer (the headline loop).
- An autonomous baseline that works with no phone: local clock, local settings, on-demand
  capture.
- Audio output through the bonded speaker.
- Wi‑Fi link to the phone; simple setup of Wi‑Fi and your AI choice.
- One-button control (press to navigate, press to ask).

**Intentionally excluded from V1** (these make it *better*, not *magic*, and come later):
- A near-eye display / HUD — **optional and experimental in V1; it becomes a real feature in
  V2.** V1 is valid and complete as an audio-only device.
- A microphone / voice trigger (the button is enough for V1).
- A polished enclosure and industrial design (V1 is honest tape-and-bracket).
- A finished mobile app (a laptop acts as the "phone brain" for the first demos).
- On-device AI, always-listening wake-word, and other heavy or privacy-costly features.

---

## 6. Future Vision (V2, V3 and beyond)

Each version is a complete, useful product for *its* audience — not a teaser for the next.
**V1 is for us, V2 is for enthusiasts, V3 is for normal people.** Nothing in V1 has to be
thrown away to reach V3; the device gets better while the autonomy, ownership, and privacy
principles stay constant.

**V2 — "the enthusiast product":** everything in V1, but *integrated* — no loose wires, a
real enclosure, a battery that lasts roughly a day, and pairing that behaves like a normal
accessory. It adds a **wearable monocular display worth keeping on** (notifications, AI
answers, navigation, live transcription), a **microphone**, a proper companion app, and a
genuinely modular/repairable build (replaceable battery, separable camera/compute/optics).

**V3 — "the normal-person product":** looks like ordinary glasses (or a barely-there clip),
all-day battery, durable. The display "disappears" — bright enough for daylight, invisible
to onlookers, glanceable. A polished assistant experience (proactive notifications,
navigation, translation/transcription, visual lookup) — still phone-brokered and
privacy-respecting — plus an open platform for third-party "screens" and apps.

**The throughline of the roadmap is increasing local capability and independence over time.**
Better displays, better optics, and stronger processors arrive in later versions — and the
long-term hardware direction explicitly contemplates much more capable, Linux-class compute
(beyond today's microcontroller) as the device grows more capable on its own. *(Use only this
established direction; do not invent specific new features.)*

---

## 7. Modularity Ecosystem

Sudonit is meant to be a **modular hardware platform**, not a single fixed gadget. Over the
long term that opens an ecosystem around the module:

- **Battery options** — different capacities and adapters for different needs.
- **Printable accessories** — community-designed, 3D-printable add-ons.
- **Replacement parts** — the battery, the clip, and other wear items are meant to be
  swapped, not discarded.
- **Custom mounts / attachment systems** — clips sized for different frame styles, so the
  module fits the glasses you actually own.
- **Community hardware** — makers building and sharing their own components.
- **Official hardware** — a first-party marketplace of parts and upgrades.

**Why modularity matters:** it is what makes the device *yours for the long run.* You can
repair it instead of replacing it, upgrade one piece instead of buying a whole new product,
and adapt it to your own glasses and your own needs. It turns a gadget into a platform —
and a platform into something a community can build on.

---

## 8. AI Philosophy

Sudonit is **privacy-friendly, not cloud-free** — and the distinction is the whole point.
The device doesn't run large AI itself (that genuinely exceeds a tiny wearable); instead the
phone brokers the request to an AI of *the user's choosing*. There are several supported
ways to get the intelligence, and the user is never locked in:

- **Your own API key** — bring your own AI account; requests go straight to the provider you
  chose. Maximum control, for makers and privacy-minded users.
- **Hosted Sudonit AI** — a convenient managed option for people who don't want to obtain or
  manage a key (planned as an optional paid service, see §9).
- **Your own / self-hosted server** — point Sudonit at a local or self-run model. A direction
  the platform deliberately supports for full data control.
- **Provider-agnostic by design** — Sudonit never depends on a single AI vendor; providers
  are replaceable, and no one provider is forced on the user.

**Privacy stance, stated accurately:** your data belongs to you. There is no unnecessary
cloud storage, no permanent retention of your images, and no selling of user data. Cloud AI
is *permitted because it's useful*, but it is always the user's choice where their data and
their AI live — from a big provider, to a hosted Sudonit service, to a server in their own
home.

---

## 9. Business Model Direction

The likely model is **buy the hardware, then optionally pay for convenience** — never a
device that dies without a subscription.

**Hardware (the foundation):**
- A one-time **device purchase**. Ownership, firmware, documentation, and all local
  functionality come with it.

**Optional paid services (convenience, never a gate):**
- **Hosted AI access** — pay Sudonit to handle the AI so you don't manage your own key.
- **Subscriptions / premium convenience features** — for people who want them.
- **A model library** — easy access to a range of AI models.
- **Cloud services** — optional sync and similar conveniences.
- **Premium support** — for those who want a safety net.

**The non-negotiable rule:** **the hardware must remain fully functional without any
subscription.** Paid services add convenience and capability on top of a device that already
works on its own (including with the user's own API key or own server). Mandatory
subscriptions are explicitly *not* part of the vision.

---

## 10. Why Sudonit Exists

Smart glasses are arriving — but the versions on sale today are closed boxes. You buy *their*
frames, run *their* cloud, accept *their* rules, and when something breaks or the company
moves on, you're left with a sealed gadget you never really owned.

Sudonit exists because that doesn't have to be the trade. Glasses are deeply personal —
people pick frames they love and wear them every day. A person should be able to make *those*
glasses smart, keep the device when they switch frames, open it up and fix it, swap the
battery, and decide for themselves whether the AI runs on a big cloud, a service they pay for,
or a server in their own home.

**Why someone would choose it:** because they want to keep their own glasses, own and repair
their hardware, and not be locked to one company's frames, cloud, or AI.

**Why open smart glasses matter:** glasses sit on your face and see what you see. A device
that intimate should be one you *control* — auditable, repairable, and not quietly dependent
on a vendor's permission to keep working. Open, owner-controlled smart glasses are a hedge
against a future where the most personal computer you own is the one you understand least.

**Why the founder is building it:** to bring the ThinkPad ethos to a category that is rushing
toward sealed, disposable, vendor-controlled hardware — to prove that smart glasses can be
open, modular, repairable, and genuinely the user's own.

---

## 11. Website Summary

**Elevator pitch (one paragraph).**
Sudonit is a small, open smart-glasses module that clips onto the glasses you already own —
no proprietary frame required. Look at something, press a button, and hear an AI answer in
your ear; meanwhile the device runs on its own, knowing the time and holding your settings
even with no phone nearby. You own the hardware, you can repair and upgrade it, you can move
it between your favorite frames, and you choose who runs the AI — your own key, a hosted
Sudonit service, or your own server. It's the ThinkPad philosophy for smart glasses: open,
modular, repairable, and genuinely yours.

**One-sentence pitch.**
Open, repairable smart glasses you clip onto the frames you already own — your hardware, your
glasses, your choice of AI.

**Five key selling points.**
1. **Keep your own glasses** — a clip-on module, not a new proprietary frame.
2. **You own it** — repairable, modular, upgradable, and yours for the long run.
3. **Your choice of AI** — your own key, hosted Sudonit AI, or your own server; never locked
   to one vendor.
4. **Works on its own** — a standalone device the phone *extends*, not a dead peripheral.
5. **No mandatory subscription** — buy the hardware and it keeps working, forever.

**Three key differentiators.**
1. **It's a portable module, not a fixed pair of glasses** — move it between any frames you own.
2. **Owner-controlled and repairable** — the ThinkPad approach, the opposite of a sealed
   consumer gadget.
3. **Provider-agnostic, privacy-friendly AI** — you decide where your data and your AI live.

---

## Appendix — for the founder (not for the website)

Things the website team will eventually need that this brief cannot yet state as fact:

**Missing information**
- **Brand & naming:** is "Sudonit" final? Any logo, wordmark, colors, or tone-of-voice
  guidance? None exists in the repository.
- **Pricing:** only an internal *parts-cost* estimate (~$25–$60 for V1) exists — there is no
  decided *retail* price for any version, and no price for the hosted AI service.
- **Availability:** no launch date, pre-order plan, or "where to buy." V1 is a prototype;
  hardware has not yet been validated on real silicon.
- **Visuals:** no product photos or renders exist — the module is still pre-hardware. Any
  imagery the website uses must be clearly marked as a concept/mockup.
- **Proof points:** real-world performance numbers (answer speed, battery life, AI accuracy)
  are not yet measured. The website should not quote any until they exist.

**Unresolved decisions**
- **Display in V1:** treated here as optional/experimental and primarily a V2 feature. If
  marketing wants to show a HUD, confirm whether V1 messaging may reference it at all.
- **Hosted Sudonit AI:** described as a *planned* optional service; it is a direction, not a
  shipping product. Confirm how forward-leaning the website may be about it.
- **Target-customer emphasis:** V1 is honestly for makers/early adopters. Decide how much the
  public site should speak to the eventual "normal person" (V3) audience vs. today's reality.

**Assumptions that still need founder confirmation**
- That **hosted AI, bring-your-own-key, and self-hosted** are all intended as first-class,
  shipping options (this brief presents them as equal peers — confirm).
- That **"no mandatory subscription, hardware works forever"** may be stated publicly as a
  firm promise.
- That the **"not competing with Ray-Ban Meta on polish; competing on openness/ownership"**
  framing is the approved public positioning.
- That **V1 is to be presented as audio-first** (no screen) without that reading as a
  limitation to the target audience.
