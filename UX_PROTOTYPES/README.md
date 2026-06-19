# UX_PROTOTYPES

Browser-based, interactive UX prototypes for the Sudonit near-eye display. **No build
step, no server** — open `index.html` (or any concept file) directly in a browser.

```
UX_PROTOTYPES/
├── index.html                        ← launcher (start here)
├── common.css                        ← shared near-eye display simulator
├── runtime-sim.html                  ← ▶ VIRTUAL DEVICE — primary product-validation tool
├── DESIGN_REVIEW.md                  ← brutally honest UX critique (uses the sim's metrics)
├── FUTURE_HARDWARE_IMPACT.md         ← each UX decision across tiny/better OLED + Linux
├── concept-a-minimal-assistant.html  ← "press, look, hear" + one-line glance
├── concept-b-ar-hud.html             ← loose (honest) AR overlay
├── concept-c-navigation.html         ← turn-by-turn HUD
└── concept-d-accessibility.html      ← live captions, user-controlled legibility
```

## The virtual device (`runtime-sim.html`)

A realistic, runnable Sudonit you can live in before hardware exists — the **primary
tool for product decisions**. It implements the runtime (`docs/UI_RUNTIME_ARCHITECTURE.md`,
`APP_MODEL.md`, `UI_STATE_MACHINE.md`) as a real experience:

- **Full flows for all 8 apps** — Camera capture, Gallery drill-down, AI Assistant,
  Notifications, Clock alarms, Calendar, Settings, Home launcher.
- **Real connectivity model** — three independent axes (**phone / internet / AI key**)
  with correct graceful degradation for every combination; flip them and watch the
  device stay useful.
- **First-time experience** — boot → Wi-Fi setup → AI-provider setup → tutorial
  (Factory reset replays it). *The AI key is never shown or stored as text.*
- **Scripted real-world scenarios A–E** — "what is this?", "read this sign", a
  notification during camera, critical battery, phone drops mid-use — auto-played.
- **Live interaction metrics** — actions/session, actions-since-Home, max nav depth,
  and a measured clicks-to-feature table (which feeds `DESIGN_REVIEW.md`).

Drive it with the on-screen buttons or the keyboard (`←/→` prev/next, `Enter` select,
`Backspace` back, `space` ask/capture, `h` home). Validated headless (16/16 behavioral
checks) before each commit.

## Reviews

- **`DESIGN_REVIEW.md`** — a product critique grounded in the simulator's measured
  metrics: UX/navigation problems, cognitive load, one-button weaknesses, and what feels
  unnecessary vs. missing. Brutally honest, with prioritized fixes.
- **`FUTURE_HARDWARE_IMPACT.md`** — every UX decision evaluated across tiny OLED, better
  OLED, and Linux-class hardware; which assumptions break upward vs. downward.

## Look studies (`concept-*.html`)

Static explorations of individual surfaces' visual design (below).

## Why they look so sparse

`common.css` deliberately constrains every concept to a **small, monocular,
monochrome, low-resolution panel** with safe eyebox margins — the actual V1/V2
hardware limit (`docs/DISPLAY_EVALUATION.md`). A design that only works full-screen
and full-colour *fails here on purpose*. Judging UX under the real constraint now is
cheaper than discovering it on silicon.

## Controls

Each concept prints its keys along the bottom. Common: **Space** = primary action.
All are keyboard/click driven and run offline.

## How they map to the architecture

The four concepts are the four content renderers in `docs/DISPLAY_ARCHITECTURE.md`
§5 (AI response, AR/HUD, navigation, accessibility/captions). Keeping the prototypes
and the renderer interfaces aligned means UX decisions made here translate directly
into the phone-side render pipeline — without touching firmware.
