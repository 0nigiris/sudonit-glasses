# PROJECT_AUDIT.md — Brutal Review

A hostile audit of Sudonit as it exists today, from six seats at once: hardware
engineer, embedded engineer, UX designer, open-source maintainer, prospective customer,
and investor. The brief was *find problems and do not defend decisions* — so this
document does not balance praise against criticism. It is the prosecution.

One framing fact governs everything below and you should keep it in mind while reading:
**not a single line of this project has ever run on real hardware.** Every driver is a
stub or an untested-on-silicon skeleton; the AI loop has never been run with a real
model; the display hardware has not been ordered. What exists is a large, coherent,
*paper* architecture and a host-side simulation of it. That is not nothing — but it is
also not progress on the only 20% that carries actual risk.

---

## 1. Weak assumptions

- **"Host-green = de-risked."** The project leans hard on "host build, host tests,
  Python tests, ESP-IDF build all green." None of that touches brownout, PSRAM
  detection, DVP camera timing, I2S noise, RF/antenna behavior, real-world Wi-Fi
  association, or real Claude latency/quality. The green checkmarks measure the *easy*
  half and create false confidence about the hard half.
- **"The camera pin map probably matches Freenove."** A guess, on an AliExpress board,
  documented as an assumption — and it gates day one. "Probably" is not an engineering
  input.
- **"PSRAM is octal because the label says N16R8."** Likely true, but the entire camera
  path depends on it and it has never been confirmed on this unit.
- **"Battery is priority #2, off the critical path."** For a wearable, battery *is* the
  product. Assuming you can validate the experience on USB power and add battery later
  assumes the device's hardest constraint is an afterthought. It is not.
- **"Users will bring their own API key."** This single assumption silently amputates
  the entire non-developer market (see §10 and the Killer Question).
- **"A clip-on is simpler than a frame."** It is the opposite. See §3 and §9.

## 2. Unrealistic expectations

- **The "magic" loop is a 5-hop chain**: capture → ESP32 → Wi-Fi → phone app → cloud AI
  → TTS → back → speaker. Realistic end-to-end latency is **2–6+ seconds** with multiple
  independent failure points (phone asleep, Wi-Fi dropped, API rate-limited, no
  internet). "Press, look, hear" sells an instantaneous feel the architecture cannot
  deliver.
- **A focusable near-eye image from a cheap OLED + beam-splitter.** The docs admit a flat
  panel 2 cm from the eye isn't focusable without collimation, then proceed as if the
  optics are a detail. They are the hard part, and they are unsolved.
- **All-day-ish battery (V2) on a behind-the-ear LiPo** driving camera + Wi-Fi + audio +
  display. The power budget has never been computed. Realistic V1 untethered runtime is
  plausibly **tens of minutes**, hot against the temple.
- **"Compete with Ray-Ban Meta."** Meta ships a vertically integrated, FCC-certified,
  good-looking, sub-3-second product at scale for ~$300. Sudonit is a solo, uncertified,
  tethered-to-USB, colorless-HUD clip-on with multi-second latency. The comparison is
  aspirational to the point of misleading.

## 3. Architectural mistakes

- **A mid-project philosophy reversal.** The architecture flipped from "phone renders
  every pixel, glasses are dumb sensors" to "autonomy-first, glasses own the UI." Both
  were written with equal confidence. A core architecture that inverts on philosophy
  rather than evidence is a warning sign: it is being driven by aesthetics, not by
  validated constraints.
- **The autonomy-first pivot massively increased scope at the worst time.** It added an
  entire on-device UI runtime, widget system, font/render stack, screen manager, 8 apps,
  a settings registry, and a notification store — *none of which exists as running code*
  — onto an MCU that must already share its resources with camera, Wi-Fi, and audio. The
  stated goal was "fastest path to first demo / risk reduction, not feature count." The
  pivot did the reverse.
- **Wi-Fi chosen as the transport because it was easy, not right.** LwIP sockets ≈ host
  TCP, so Wi-Fi shipped first. For a battery wearable, Wi-Fi STA is power-hungry and
  needs a network; BLE (the originally "preferred" link) is *unstarted*. The easy path
  was paved; the correct path was deferred into debt.
- **The display, the least-certain subsystem, received the most design.** UI runtime, app
  model, state machine, bring-up plan, four UX concepts, a 530-line simulator, two review
  docs — all targeting a panel that hasn't been bought and optics that may not work.

## 4. Unnecessary complexity

- **Eight built-in apps** (incl. on-device Gallery browsing, multiple clock faces) for a
  256×64 one-button device, when the product's whole reason to exist is one flow ("ask").
  Gallery browse costs a measured **6 actions** and competes with the phone you're holding.
- **A schema-driven settings registry, notification ring buffer, calendar cache, and
  generic widget/layout engine** — a small embedded GUI framework — designed before the
  device has shown one pixel. This is framework-building ahead of need.
- **A simulator that models Power/UI/Overlay/Link orthogonal regions** in 530 lines of JS
  for a product whose first demo will likely be "does the camera return a JPEG."

## 5. Hidden technical debt

- **BLE control plane: 0% built**, but assumed by the display stream and the "pairs like
  an accessory" story.
- **Battery/charging/fuel-gauge: 0% built**; "USB for the demo" is debt with interest.
- **A real phone app: does not exist.** There is a Python server. A consumer product needs
  a maintained iOS + Android app — two platforms, app-store review, BLE stacks, background
  execution limits. That is a project on its own and it is invisible in the plan.
- **The simulator validates a fiction.** Its own review docs admit it uses color, emoji,
  and comfortable multi-line text the 1-bit panel cannot render. The "primary validation
  tool" is partly validating an experience that will not exist.
- **Real Claude integration is a single untested function** behind a missing key. Image
  format, multimodal quality on SVGA, cost-per-query, refusal behavior, and latency are
  all unknown unknowns sitting under the value proposition.

## 6. Scalability problems

- **The launcher is O(n).** Seven apps already cost up to 3 cycles to reach; the roadmap
  *adds* apps. One-button + linear list does not scale with the feature ambitions.
- **ESP32-S3 resource ceiling.** SVGA JPEG framebuffer in PSRAM + Wi-Fi stack + I2S
  buffers + an on-device UI runtime with fonts and a framebuffer + protocol heap buffers,
  all in 8 MB PSRAM / limited internal RAM, concurrently. Nobody has measured whether the
  full configuration even fits and runs, let alone with headroom.
- **No volume story.** "Cheap BOM" is asserted at qty 1 from AliExpress. At any real
  volume there is no manufacturing partner, no panelization, no test fixture, no yield.

## 7. Maintainability problems

- **Doc-to-running-code ratio is inverted.** The repository is rich in architecture and
  poor in code that executes on the target. Maintaining a large paper design that the
  hardware may invalidate is negative work.
- **Bus factor = 1.** One person holds the vision, the firmware, the protocol, the UX, the
  simulator, the hardware, and the (nonexistent) app. There is no second contributor, no
  community, and the surface area already exceeds what one person can keep current — the
  mid-project pivot already left the top-level docs temporarily contradicting the new ones.
- **Two languages, one protocol, byte-compatibility maintained by hand** (C ↔ Python).
  Elegant, but every protocol change is a two-sided, hand-verified edit — fine for now,
  a tax forever.

## 8. Reliability risks

- **Brownout on first power-up** (camera + Wi-Fi current spike) is the single most likely
  day-one failure and is acknowledged but unmitigated beyond "power from USB, measure."
- **The loop has no graceful story for the common failure**: phone asleep / Wi-Fi dropped
  mid-ask. The device shows "AI unavailable," but for a product whose only trick is asking,
  "unavailable" *is* the typical experience away from a known network.
- **Thermal**: warm compute behind the ear, against skin, in a small enclosure, with no
  measured thermal budget.

## 9. Manufacturing risks

- **Certification is entirely absent from the project.** A wireless (2.4 GHz) camera
  wearable needs FCC/CE; the LiPo needs UN38.3 for shipping; selling a recording device
  invites privacy-law exposure (two-party consent states, GDPR for bystanders). None of
  this is acknowledged anywhere.
- **User-assembled clip-on = liability and support nightmare.** A maker selling a kit that
  clamps a LiPo and a camera onto someone's prescription glasses owns every failure mode.
- **Fit variability is unbounded.** "Attaches to your existing glasses" means infinite
  temple geometries, weights, and face shapes — each needing the eyebox aligned. There is
  no manufacturable "one clip fits all."

## 10. User-experience risks

- **A conspicuous face camera on a clip-on is *more* socially awkward than an integrated
  product, not less.** The Glass lesson is cited but the form factor worsens it.
- **Multi-second latency on the one core flow** makes the magic feel broken the third time.
- **One-button secondary navigation is expensive** (photo = 5 actions, brightness = 4) and
  text entry (Wi-Fi password, API key) is effectively impossible on-device — setup *must*
  be phone-assisted, which the flow currently oversells as on-device.
- **A colorless 256×64 HUD** cannot render the simulator's experience; real legibility of
  AI answers near-eye in daylight is unproven and probably poor.
- **BYO-API-key is a UX wall** for anyone who is not a developer.

---

## The uncomfortable summary

Sudonit is, today, an **elegantly over-designed paper product with an unproven core**.
The engineering taste is real (clean HAL split, host-test discipline, a coherent
protocol). But the project has spent its energy on the parts that are *pleasant and
certain to design* (architecture, UI runtime, simulator) and deferred the parts that are
*hard and likely to hurt* (hardware bring-up, power, optics, real AI, and — above all —
whether anyone but a developer would want this). The risk is not that the code is bad.
The risk is that the code is beside the point.

---

## Killer Question — the single biggest reason this fails even if all code works perfectly

> **Sudonit is technology-led, not demand-led, and the one differentiator it actually has
> — openness with your own API key — is precisely the feature that guarantees it has no
> market beyond a few hundred developers.**

Even with flawless firmware, a focusable display, and a 1-second loop, the honest
question is: *why would anyone choose this over a Ray-Ban Meta?* Meta's is cheaper to
own (no API key, no per-query cost), prettier, certified, supported, lower-latency, and
sold in stores. Sudonit's answer is "it's open and private." That answer moves a small,
devoted niche and **no one else** — and that niche will not sustain a solo maintainer, a
two-platform app, hardware certification, or a supply chain. The project can succeed as
an *open-source learning artifact and a developer's platform*; it cannot succeed as a
*product* on its current premise, no matter how good the code gets. The fatal flaw is in
the business and demand model, not the bug tracker — which is exactly why no amount of
perfect code can fix it.
