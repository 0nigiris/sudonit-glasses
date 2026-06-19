# UX_PROTOTYPES

Browser-based, interactive UX prototypes for the Sudonit near-eye display. **No build
step, no server** — open `index.html` (or any concept file) directly in a browser.

```
UX_PROTOTYPES/
├── index.html                        ← launcher (start here)
├── common.css                        ← shared near-eye display simulator
├── concept-a-minimal-assistant.html  ← "press, look, hear" + one-line glance
├── concept-b-ar-hud.html             ← loose (honest) AR overlay
├── concept-c-navigation.html         ← turn-by-turn HUD
└── concept-d-accessibility.html      ← live captions, user-controlled legibility
```

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
