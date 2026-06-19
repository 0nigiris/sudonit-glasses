# FUTURE_HARDWARE_IMPACT.md

The virtual device (`runtime-sim.html`) makes UX decisions under *today's* assumptions:
a tiny 1-bit monocular OLED and one button. This document stress-tests each decision
across three hardware tiers and flags **which assumptions break when hardware improves**
(and, just as important, which the simulator already breaks by being too generous about
V1).

The three tiers (DISPLAY_EVALUATION.md, PRODUCT_VISION.md):

| Tier | Display | Input | Compute |
|---|---|---|---|
| **T1 · Tiny OLED (V1)** | ~256×64, **1-bit mono**, monocular, small FOV | **1 button** | ESP32-S3 |
| **T2 · Better OLED (V2)** | larger, gray/▸color microLED-waveguide, monocular | button + touch bar / encoder; mic | ESP32-S3 class |
| **T3 · Linux-class (V3)** | large or external, full color, rich | touch / voice / external | RPi/Linux, GPU |

Legend: ✅ holds · ⚠️ strained · ❌ breaks.

---

## Per-decision evaluation

### D1 · Color & emoji to carry meaning (warn/error/focus, 📷🔔✦)
- **T1:** ❌ **Breaks hard.** No color, no emoji on 1-bit mono. The simulator's amber/red
  and glyph icons are *not real on V1*. Must re-encode as shape/inverse/position/blink +
  1-bit icon bitmaps. **This is the #1 assumption the sim violates downward.**
- **T2:** ⚠️ gray levels / limited color return some of it; icons feasible. Partial relief.
- **T3:** ✅ full color/emoji/icons trivially.
- **Assumption that breaks:** "meaning can ride on color." True at T3, false at T1.

### D2 · Linear ring launcher (O(n) to reach an app)
- **T1:** ⚠️ tolerable at 7 apps; degrades as the app list grows (already 5 actions to a
  photo). Input scarcity makes the linear cost painful.
- **T2:** ⚠️→✅ a touch bar/encoder makes cycling cheap and adds direct-ish selection.
- **T3:** ✅ grid + search + pointer/voice; linear model is obsolete.
- **Assumption that breaks:** "few apps, sequential is fine." Breaks *upward* as the
  roadmap adds apps, and is rescued more by better **input** (T2) than a better display.

### D3 · One-button input model (short/long/double = Select/Back/Next)
- **T1:** ✅ necessary and (barely) sufficient; the floor.
- **T2:** ⚠️ keeping *only* one button here would waste available input; touch/encoder/
  voice should supersede it. The model should *expand*, not persist unchanged.
- **T3:** ❌ one-button thinking is actively wrong; voice/touch are primary.
- **Assumption that breaks:** "input is scarce." Breaks upward at T2/T3. **Protected** by
  the logical-action abstraction (UI_RUNTIME §5) — *if* the UX avoids one-button-only
  patterns (it currently bakes some in: overloaded PRIMARY, linear launcher — see
  DESIGN_REVIEW). Fix those and the abstraction carries the design upward cleanly.

### D4 · Text quantity per screen (answers, notes, agenda rows)
- **T1:** ❌ over-budget; most AI answers/notification bodies truncate or scroll on 256×64.
  The sim's two-line answers are optimistic.
- **T2:** ⚠️ more rows; still small FOV → summaries, not essays.
- **T3:** ✅ full text, scrollback, even markdown/links.
- **Assumption that breaks:** "the screen can show a sentence or two." Marginal at T1,
  comfortable only at T3. Drives a need for phone handoff at T1 (see D8).

### D5 · On-device Gallery browsing
- **T1:** ❌ low value (6 actions, tiny mono thumbnails) — worse than the phone you carry.
- **T2:** ⚠️ nicer thumbnails, still redundant with the phone.
- **T3:** ✅ a real gallery makes sense on a big display.
- **Assumption that breaks:** "the device is the best screen for reviewing media." Never
  true at T1; only true at T3. Argues to defer browse and add handoff regardless of tier.

### D6 · Dense Home glance (time+event+unread+battery+AI status)
- **T1:** ⚠️ cramped; borderline "glanceable."
- **T2:** ✅ more room; can add complications.
- **T3:** ✅ a full dashboard if wanted.
- **Assumption that breaks:** none structurally — it *scales up* gracefully. The fix at
  T1 is to *trim*, and the design already adapts via `DisplayProfile`.

### D7 · Overlays (auto-dismiss notifications, modal low-battery)
- **T1:** ⚠️ forced by scarcity — no room for a persistent notification region, so it
  must overlay and time out (and risks being missed).
- **T2:** ✅ enough room for a persistent status/notification strip → fewer modal steals.
- **T3:** ✅ non-modal toasts, notification center, multitasking.
- **Assumption that breaks:** "there's no room for a persistent secondary region." True at
  T1, false at T2+. So the *auto-dismiss compromise* is a T1 artifact, not a principle —
  don't enshrine it.

### D8 · Phone as text-entry & long-form surface (setup, "open on phone")
- **T1:** ✅✅ essential — one button can't enter a Wi-Fi password or render a long answer.
- **T2:** ✅ still valuable for long-form; less for input once a touch bar exists.
- **T3:** ⚠️ the device itself can do text/long-form; the phone dependency *relaxes*.
- **Assumption that breaks (the interesting one):** "the device needs the phone for text
  and length." True and load-bearing at T1; **weakens as hardware improves** — at T3 the
  glasses could be self-sufficient for input too. The architecture's "phone extends"
  framing stays correct (AI/cloud always extend), but the *input/text* dependency is a
  T1 fact, not a forever fact.

### D9 · Monochrome focus cue (border/inverse highlight)
- **T1:** ✅ inverse/border works in 1-bit (once D1's color highlight is replaced by it).
- **T2/T3:** ✅ plus color. Holds across all tiers — the *right* primitive to standardize on.

### D10 · AI as the one phone-required feature; everything else local
- **T1–T3:** ✅ holds at every tier — autonomy-first is hardware-independent by design.
  Better hardware lets *more* run locally (even light on-device models at T3), only
  strengthening the principle. **The one decision that never breaks.**

---

## Summary: assumptions by failure direction

**Break DOWNWARD (the simulator is too generous about V1 — fix before trusting the sim):**
- **D1 color/emoji** — the worst offender; V1 is 1-bit, colorless, glyph-only.
- **D4 text quantity** — V1 truncates; design for summaries + handoff.
- **D7 overlay room** — V1 has none; auto-dismiss is a constraint artifact.

**Break UPWARD (don't over-optimize for V1 scarcity; leave room to grow):**
- **D3 one-button** and **D2 linear launcher** — both must expand with better input;
  protected only if the UX stops baking in one-button-only patterns (DESIGN_REVIEW #5,#8).
- **D8 phone-for-text dependency** — load-bearing at T1, relaxes by T3.

**Hold across all tiers (safe to standardize on now):**
- **D6 glance**, **D9 mono focus cue**, **D10 autonomy-first** — these scale cleanly and
  should be treated as durable.

**Practical guidance for the next simulator iteration:** add a **"V1-honest" mode** that
forces 1-bit, strips color/emoji, caps text rows, and renders icons as bitmaps — so the
virtual device stops flattering hardware it won't have. Keep the richer rendering as a
"T2/T3 preview." That single toggle would make every decision above testable under the
tier it actually targets, which is the whole point of validating in software first.
