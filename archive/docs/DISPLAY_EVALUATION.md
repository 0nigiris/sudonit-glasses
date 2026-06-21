# DISPLAY_EVALUATION.md

Realistic near-eye display options for Sudonit, evaluated for **what one person
can actually source, drive from an ESP32-S3, and wear on a clip-on module** — not
for what a funded optics team could build.

Two hard constraints frame everything below:

1. **The MCU is an ESP32-S3.** It can drive SPI and 8/16-bit parallel (the LCD/RGB
   peripheral) panels at modest resolution. It **cannot** natively output MIPI-DSI,
   LVDS, or DisplayPort, and it cannot drive an LCOS / microLED projector's ASIC.
   So whatever display we pick must accept a simple SPI/parallel pixel stream, or
   bring its own driver board that does.
2. **Glasses are dumb sensors; the phone is the brain** (ARCHITECTURE.md). The phone
   should render frames and send a small bitmap downlink; the ESP32 just blits it.
   This keeps the display option from also dragging in a rendering engine on the MCU.

A third, softer constraint: Sudonit is a **clip-on module for existing glasses**
(VISION.md). The optic has to be small, light, and mountable off-axis — which rules
out anything that needs a custom-moulded lens.

> **Vocabulary correction up front:** "LCOS", "microLED", and "microOLED" are *image
> sources*. "Waveguide", "birdbath", and "prism/combiner" are *optics* that relay
> that image to the eye. A real display is always **source + optic**. The list below
> is organized by source but always names the optic it implies, because the optic is
> usually the harder half to source.

---

## Option A — Small transparent / passthrough OLED panel

A small OLED (0.23"–0.49", monochrome or RGB, SSD1306/SSD1309/SH1107-class or a
0.39" 256×64) reflected into the eye with a **beam-splitter prism** or angled
semi-mirror. This is the Google-Glass-style "glance display."

| | |
|---|---|
| **Source** | Small SPI/I2C OLED, $3–$15, everywhere |
| **Optic** | 45° beam-splitter cube or thin acrylic combiner, $5–$30 |
| **ESP32 link** | SPI, trivially supported (existing OLED drivers) |
| **Driver complexity** | **Low** — `u8g2`/native SPI; we already speak SPI |
| **Power** | ~10–40 mW (monochrome), panel is tiny |
| **Latency** | Sub-frame; SPI blit of a 256×64 1-bit buffer is microseconds of data |
| **Availability** | Excellent (AliExpress, Mouser, EU resellers) |
| **Cost range** | **$10–$45** all-in |

**Pros:** cheapest, simplest, sourceable today, low power, the ESP32 drives it with
code we essentially already have. Honest "HUD glance" UX. Easy to prototype on a
breadboard before any optics.
**Cons:** the optic is the hard part — getting a crisp, focused virtual image from a
flat panel + beamsplitter at near-eye distance needs a lens; without collimation the
eye can't focus on something 2 cm away. Small FOV (~10–20°), monochrome or dim
color, see-through contrast poor in daylight.
**Difficulty:** Low (electronics) / Medium (optics).
**V1 suitability:** **High.** **V2 suitability:** Low (looks like a prototype).

---

## Option B — Off-the-shelf monocular HUD module (integrated source + optic)

Pre-built near-eye modules sold for DIY smart glasses: a microdisplay already
bonded to a prism or birdbath combiner, often with a tiny driver board. Includes
the cheap monochrome "green HUD" modules and salvaged Google-Glass-style prisms on
AliExpress, plus hobbyist kits.

| | |
|---|---|
| **Source** | Module-dependent (often microOLED/LCD) |
| **Optic** | Bundled prism/birdbath — *the reason to buy these* |
| **ESP32 link** | Varies: SPI (good), or MIPI/HDMI (**dealbreaker** for ESP32) |
| **Driver complexity** | Low *if* SPI; **High/impossible** if MIPI-DSI/HDMI |
| **Power** | 50–250 mW |
| **Latency** | Low if SPI; module-dependent |
| **Availability** | Patchy, listings appear/vanish, QC varies |
| **Cost range** | **$25–$120** |

**Pros:** someone else solved the optics — the part we're worst-equipped to do. Some
are genuinely wearable. Fast path to "image floating in front of eye."
**Cons:** sourcing roulette; many need MIPI/HDMI input the ESP32 can't produce, so
you'd add an HDMI-bridge board (cost, power, bulk). Documentation usually nonexistent.
**Difficulty:** Medium (gated entirely on picking an SPI-input module).
**V1 suitability:** **Medium–High** (if an SPI module is found). **V2:** Medium.

---

## Option C — Reflective LCOS microdisplay + projection optics

LCOS panel (reflective liquid crystal, e.g. Himax/Syndiant class) illuminated by an
LED, projected through relay optics into a birdbath or waveguide. This is what a lot
of commercial AR uses.

| | |
|---|---|
| **Source** | LCOS panel + illumination, hard to buy as a single unit |
| **Optic** | Projection lens + combiner (birdbath or waveguide) |
| **ESP32 link** | Needs a dedicated LCOS driver ASIC (parallel RGB/MIPI) |
| **Driver complexity** | **Very high** — illumination timing, driver ASIC, optics alignment |
| **Power** | 200 mW–1 W+ (LED illumination dominates) |
| **Latency** | Low once driven, but driving it is the problem |
| **Availability** | **Poor** for single units; B2B / dev-kit only |
| **Cost range** | **$150–$600+** |

**Pros:** bright, high-resolution, full color; the real path to a consumer-grade
image.
**Cons:** essentially un-sourceable and un-driveable for a solo builder. Needs an
illumination engine and a driver chip the ESP32 cannot replace. Optical alignment is
a lab task.
**Difficulty:** Very High.
**V1 suitability:** **None.** **V2 suitability:** Low–Medium (only via a bought engine).

---

## Option D — MicroLED microdisplay (e.g. JBD-class) + waveguide

Monolithic microLED microdisplay — extremely bright, tiny, very low power, commonly
**monochrome green** at the affordable end — paired with a diffractive/geometric
**waveguide**. This is the current high-end direction (Even Realities, Vuzix, RayNeo).

| | |
|---|---|
| **Source** | MicroLED microdisplay, **B2B, MOQ, NDA**, very hard for individuals |
| **Optic** | Waveguide — precision glass, expensive, not hobbyist-sold |
| **ESP32 link** | Dedicated driver IC; not a raw ESP32 target |
| **Driver complexity** | **Very high** |
| **Power** | Display itself very efficient (~tens of mW), but driver/system more |
| **Latency** | Low |
| **Availability** | **Very poor** for one-offs; this is a supply-chain play |
| **Cost range** | **$300–$1500+** (panel + waveguide) |

**Pros:** the genuinely "magical," normal-looking-glasses endgame: bright enough for
daylight, sips power, invisible to onlookers.
**Cons:** you cannot buy one waveguide + one microLED and wire it to an ESP32. This is
a manufacturing partnership, not a BOM line. Monochrome green at the affordable tier.
**Difficulty:** Very High / supply-gated.
**V1 suitability:** **None.** **V2 suitability:** Medium (aspirational, partner-gated).

---

## Option E — Waveguide combiner (optic only, paired with B/C/D)

Listed separately because people ask for "a waveguide" as if it were a display. It
is **only the combiner**: a flat glass/plastic plate with in/out couplers that pipes
a projected image to the eye. It needs a projector engine (LCOS/microLED) feeding it.

**Pros:** thinnest, most normal-looking optic; the reason commercial AR glasses look
like glasses. **Cons:** expensive, low light efficiency (you lose most of the
projector's light), narrow eyebox, near-impossible to source as a single quality unit
for a hobbyist, and useless without a projector Sudonit can't drive. **Difficulty:**
Very High. **V1:** None. **V2:** Medium (only as part of a bought engine).

---

## Option F — Monocular vs binocular (architecture choice, not a panel)

Orthogonal to source/optic: do we put a display on one eye or both?

| | Monocular | Binocular |
|---|---|---|
| **Cost / power** | 1× | ~2× display + 2× optics |
| **Complexity** | One image source, one optic | Two engines + convergence/IPD tuning + frame sync |
| **UX fit** | Glance HUD: notifications, text, nav arrow | Immersive overlay, stereo depth |
| **Eye comfort** | Some users dislike one-eye, but fine for glances | More natural for sustained use, harder to get right |
| **Solo-builder fit** | **Strong** | Weak for V1 |

**Verdict:** Sudonit is a *glance assistant*, not an immersive AR headset. **Monocular
for V1 and V2.** Binocular is a V3+ research bet, not a near-term goal. A clip-on
module is inherently monocular anyway.

---

## Side-by-side summary

| Option | Source+Optic | ESP32-driveable | Difficulty | Cost | V1 | V2 |
|---|---|---|---|---|---|---|
| **A** Small OLED + beamsplitter | OLED + prism | **Yes (SPI)** | Low/Med | $10–45 | **High** | Low |
| **B** OTS HUD module | bundled | If SPI | Med | $25–120 | **Med–High** | Med |
| **C** LCOS + projection | LCOS + birdbath/wg | No (ASIC) | Very High | $150–600 | None | Low–Med |
| **D** MicroLED + waveguide | µLED + waveguide | No (ASIC) | Very High | $300–1500 | None | Med |
| **E** Waveguide (optic only) | — needs C/D | No | Very High | high | None | Med |
| **F** Monocular vs binocular | architecture | — | — | — | **Mono** | **Mono** |

---

## Recommended display stack

### Prototype V1 — **Option A, monocular, monochrome**
Small SPI OLED (start with a 0.42"–0.49" panel, e.g. SSD1306/SSD1309-class; target a
0.39" 256×64 for more text) + a 45° beam-splitter combiner, one eye. Driven directly
by the ESP32-S3 over SPI; the phone sends a packed 1-bit bitmap downlink and the
device blits it.

**Why:** it is the *only* option a solo builder can source, drive, and wear this
quarter. It is cheap, low-power, and reuses our SPI competence. It proves the entire
display pipeline (phone render → downlink → blit → focusable virtual image) end to
end. **Honest expectation:** small FOV, monochrome, needs a collimating lens to be
focusable; daylight contrast will be mediocre. That's acceptable — V1's job is to
validate the pipeline and the UX, not to look like a product.

**Explicit fallback:** if the optics prove too fiddly for the first demo, ship V1
**audio-first with no display** (this is exactly what Ray-Ban Meta does and it still
feels magical — see MVP_DEFINITION.md) and treat the OLED as a parallel track. The
display subsystem is designed so the display is an *optional output*, never a
dependency.

### Consumer-oriented V2 — **Option B → bought microLED/waveguide engine (D)**
Move to an **integrated engine with the optics solved by the vendor**: ideally a
**monochrome-green microLED + waveguide engine** sourced as a module (Option D, but
bought, not built), or a well-documented SPI-input HUD module (Option B) as the
stepping stone. Still monocular, still text/HUD-class content — matching Even Realities'
proven "subtle green readout in normal-looking glasses" niche rather than chasing
full-color immersive AR.

**Why:** V2's differentiator is *wearability and being open*, not pixel count. A bright,
power-frugal, normal-looking monochrome readout beats a dim full-color birdbath that
makes the glasses look like a prototype. Crucially, **the ESP32 side does not change**:
the phone still renders frames and sends a bitmap; only the panel/optic and its driver
board change. The architecture (DISPLAY_ARCHITECTURE.md) makes that swap a backend
change, not a rewrite.

**Not in the V1/V2 plan:** LCOS, binocular, full-color waveguide, custom optics. They
are V3 research bets, documented in PRODUCT_VISION.md, deliberately postponed.
