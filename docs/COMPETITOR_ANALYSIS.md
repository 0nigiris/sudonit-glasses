# COMPETITOR_ANALYSIS.md

Where Sudonit sits among real smart-glasses products, and the gap it can credibly
fill. Prices and capabilities are as understood at time of writing (2026) and move
fast — treat them as directional. The point is not a spec sheet; it is **finding the
hole in the market a one-person/small-team open project can actually occupy.**

---

## The field

### Meta Ray-Ban (Gen 2, and Ray-Ban Display)
- **Display approach:** base line is **audio-only, no display** (and it still sells —
  the key lesson). The newer *Display* model adds a monocular in-lens screen with a
  wristband (EMG) controller.
- **AI:** strong — Meta AI, multimodal "look and ask," conversational, cloud-backed.
- **Strengths:** genuinely good camera + audio, real eyewear styling (Ray-Ban/Oakley),
  mature app, scale.
- **Weaknesses:** closed, Meta-owned cloud + data, not repairable, not hackable, your
  data is the product.
- **Price:** ~$300 (base) / ~$800 (Display + band).

### Google Glass
- **Display approach:** monocular prism, ~640×360 glance HUD. Historically important.
- **AI:** minimal by today's standards; pre-LLM era.
- **Strengths:** pioneered the glance-HUD form factor; the prism optic is the
  spiritual ancestor of Sudonit V1's Option A.
- **Weaknesses:** **discontinued** (Enterprise Edition wound down). Social rejection
  ("Glasshole") is a cautionary tale about cameras and norms.
- **Price:** n/a (defunct).

### XREAL (Air 2 / One / Ultra)
- **Display approach:** **birdbath OLED, binocular**, ~1080p/eye, large virtual screen.
  Tethered display glasses (DisplayPort-over-USB-C).
- **AI:** essentially none on-device; it's a *display*, not an assistant. AI is whatever
  the tethered host runs.
- **Strengths:** excellent image quality and FOV for the price; great for media/desktop.
- **Weaknesses:** **tethered**, not standalone, no always-on camera-assistant loop,
  battery/compute lives on the host. Different product category.
- **Price:** ~$400–$700.

### Brilliant Labs Frame
- **Display approach:** monocular color microdisplay + geometric prism, small FOV.
- **AI:** **AI-first and open** — multimodal assistant, open-source SDK, hackable.
- **Strengths:** the closest thing to Sudonit's philosophy: **open, AI-centric,
  developer-friendly, small.** Proves an open AI-glasses project can ship.
- **Weaknesses:** tiny/dim display, limited battery, niche; small FOV limits UX.
- **Price:** ~$350.

### Even Realities G1
- **Display approach:** **monochrome green microLED + waveguide**, very subtle,
  monocular/binocular; looks like *normal glasses*. No camera.
- **AI:** light assistant features; the display is the star (teleprompter, navigation,
  notifications, translation as text).
- **Strengths:** **the wearability benchmark** — genuinely passes as ordinary eyewear;
  the green-text HUD is exactly Sudonit's V2 display target.
- **Weaknesses:** closed, no camera (so no "look and ask"), monochrome only, premium
  price.
- **Price:** ~$600–$800.

### Also relevant
- **Vuzix / RayNeo (TCL) / Rokid / INMO** — microLED-waveguide, enterprise or
  standalone-Android; more compute on-board, heavier, pricier, closed. They show where
  the *display tech* is going (V2/V3 reference) but not the open/cheap niche.
- **Snap Spectacles ('24)** — capable standalone AR dev kit, bulky, dev-only, expensive.
- **Halliday / others** — tiny in-frame "glance" displays; validate the minimal-HUD UX.
- **Humane AI Pin** — *not glasses*, but the cautionary tale: a standalone AI device
  with on-board compute and its own cloud failed on cost, heat, battery, and latency.
  **Strong evidence for Sudonit's "let the phone do it" thesis.**

---

## Comparison at a glance

| Product | Display | AI | Open? | Camera | Price |
|---|---|---|---|---|---|
| Ray-Ban Meta (base) | none (audio) | strong, closed cloud | ✗ | ✓ | ~$300 |
| Ray-Ban Display | mono in-lens | strong, closed | ✗ | ✓ | ~$800 |
| Google Glass | mono prism | minimal | partly | ✓ | defunct |
| XREAL | binocular birdbath | none (it's a screen) | ✗ | some | $400–700 |
| Brilliant Frame | mono color prism | **AI-first, open** | **✓** | ✓ | ~$350 |
| Even Realities G1 | **mono green waveguide** | light | ✗ | ✗ | $600–800 |
| Vuzix/RayNeo/etc. | µLED waveguide | varies | ✗ | varies | $$$ |
| Humane AI Pin | (pin, projector) | on-device, closed | ✗ | ✓ | failed |
| **Sudonit** | **optional mono (A→µLED)** | **phone-brokered, your key** | **✓✓** | **✓** | **$25–180 BOM** |

---

## The gaps Sudonit can fill

1. **Open + camera + AI, all three.** Frame is open + AI but display-centric; Even
   Realities is wearable but closed and camera-less; Ray-Ban is camera+AI but closed.
   **No one offers open-source, camera-equipped, AI-assistant glasses you fully own.**
2. **Privacy by construction.** "The phone is the brain, with *your* API key" means no
   vendor cloud owns the camera feed. That is a real, marketable difference from every
   closed competitor — and increasingly the thing buyers care about.
3. **Clip-on to existing glasses.** Everyone else sells a *frame*. Sudonit attaches to
   the glasses you already wear and like (VISION.md). Lower cost, lower commitment,
   uniquely accessible — nobody serious is doing this.
4. **Repairable + modular.** Replaceable battery, separable camera/compute/optics. The
   entire industry trend is the opposite (glued, sealed, disposable). This is a values
   wedge, not just a feature.
5. **Radically low BOM.** Because there's no on-board AI silicon and no proprietary
   cloud, the BOM is a fraction of the competition. That funds the open/repairable
   model instead of fighting it.
6. **The Humane lesson, applied.** Don't put the AI (and its heat/battery/latency) on
   the wearable. Sudonit's architecture already does the opposite. That's not a feature
   to add — it's a mistake we've structurally avoided.

**Where Sudonit will *not* win, and shouldn't try:** image quality vs XREAL, polish/
scale vs Meta, optical subtlety vs Even Realities (in V1). The strategy is not to
out-spec anyone — it's to be **the open, private, cheap, repairable, clip-on AI
glasses that no closed vendor will ever make.** That niche is empty and defensible.
