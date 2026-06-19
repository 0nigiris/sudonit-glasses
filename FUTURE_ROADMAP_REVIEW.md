# FUTURE_ROADMAP_REVIEW.md

A hostile evaluation of the V1 → V2 → V3 roadmap (PRODUCT_VISION.md). For each version:
what genuinely becomes possible, what stays impossible, which assumptions break, and what
should be redesigned. The recurring theme: **the roadmap assumes a resource trajectory
— team, money, supply chain, certification — that a solo open project does not have, and
the jump between versions is where the real walls are, not within them.**

---

## V1 — ESP32-S3 (the current target)

**What becomes possible**
- The core capture → phone → AI → audio loop on real hardware.
- A genuinely autonomous local device (clock/settings/notes/camera) — the part most worth
  having and most likely to actually work.
- A bench/dev platform others can fork and learn from.

**What remains impossible**
- A *product*. V1 is tethered to USB power (no real battery story), probably display-less
  (panel not ordered, optics unsolved), and driven by one button no normal user will learn.
- Daily wearability — power, heat, and form factor aren't there.
- Anyone but a developer using it — BYO-key, CLI server, no app.

**What assumptions break**
- "Host-green ≈ ready" breaks the instant power, brownout, and real latency are measured.
- "Display is a parallel track that can catch up" breaks because the optics are a research
  problem, not a sprint.
- "Fastest path to first demo" broke retroactively when autonomy-first added a UI runtime.

**What should be redesigned (or rather, *cut*)**
- **Cut to one flow.** V1 should be: button → capture → AI → audio, plus a local clock.
  Drop the 8-app launcher, gallery, settings registry, and most of the UI runtime from the
  V1 critical path. They are V2+ *if the display ever earns its place*.
- Replace "we'll build near-eye optics" with "buy a known-good HUD module or ship audio-only."
- Make first-run **phone-assisted** and stop pretending on-device text entry is viable.

---

## V2 — "Better hardware" (custom PCB, integrated battery, real display, BLE)

**What becomes possible**
- A device that survives a day and pairs like an accessory (BLE) — *if* the power and
  display problems are actually solved.
- A bought microLED/waveguide or documented HUD engine could make the display real.
- A maintained companion app could open it to enthusiasts.

**What remains impossible**
- For a solo maker: **sourcing a microLED/waveguide engine in small quantity** (B2B, MOQ,
  NDA), **designing and manufacturing a custom PCB + enclosure + optics**, and **shipping
  a two-platform app** — simultaneously. Each is a project; together they are a company.
- Certification (FCC/CE/UN38.3) without budget and lab access.

**What assumptions break**
- "The ESP32 side doesn't change; only the panel/driver swaps" breaks if the display engine
  demands an interface the ESP32 can't drive (most good ones do) — then the *whole compute
  choice* is back on the table.
- "Wi-Fi now, BLE later as an optimization" breaks: BLE becomes load-bearing (power,
  pairing, the display stream) and it's still 0% built.
- "Repairable/modular" collides with "integrated, all-day, good-looking" — those pull
  against each other physically.

**What should be redesigned**
- Treat V2 as **a team-and-supply-chain milestone, not a firmware milestone.** The blocking
  work is sourcing, mechanical, app, and cert — none of which is in the current plan.
- Decide BLE-first *now*; Wi-Fi was a bench convenience.
- Re-evaluate whether ESP32-S3 is still the right SoC once a real display engine is chosen.

---

## V3 — Linux-class hardware ("disappears into normal glasses", maybe binocular)

**What becomes possible**
- Rich display, voice, on-device light models, a real OS and app ecosystem — the design
  constraints of V1/V2 largely dissolve.
- A device that could genuinely compete on experience.

**What remains impossible (for this project as constituted)**
- Custom optics manufacturing, industrial design, supply chain at volume, an ecosystem, and
  ongoing support — this is a funded hardware company, full stop. A solo open project does
  not reach here on its own.
- The cost target ($150–350 open + repairable) against vertically integrated incumbents.

**What assumptions break**
- **The entire V1/V2 UX is over-fitted to scarcity.** One-button, 256×64, monochrome,
  phone-for-everything — almost none of it should survive to V3. The logical-action and
  DisplayProfile abstractions protect *some* of it, but the app model, navigation, and
  rendering would be redesigned, not ported.
- "Autonomy-first" *strengthens* (more runs locally) — the one assumption that survives.
- "The compute/protocol/privacy model carries through unchanged" breaks: a Linux SoC, a
  different transport, and likely a different AI strategy mean V3 shares philosophy with
  V1, not code.

**What should be redesigned**
- Honestly, V3 is **a different product** wearing the same values. Treat it as a north star,
  not a continuation. Designing for it now is premature.

---

## The cross-version truth

- **Within each version, the firmware is the easy part.** The walls are *between* versions:
  V1→V2 is "become a hardware manufacturer + app developer"; V2→V3 is "become a funded
  company." The roadmap quietly assumes those transitions are free.
- **The one durable thread is autonomy-first**, which is hardware-independent and gets
  stronger over time. Almost everything else — input model, display assumptions, transport,
  app set — is V1-scarcity-shaped and should be expected to be redesigned, not carried.
- **Recommendation:** stop planning V2/V3 in detail. Earn the right to plan V2 by getting a
  *measured*, *battery-powered*, *reliable* single-flow V1 onto a face. Until that exists,
  V2/V3 are fiction with a version number.
