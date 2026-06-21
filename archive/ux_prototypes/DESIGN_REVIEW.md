# DESIGN_REVIEW.md

A product review of the Sudonit experience as built in `runtime-sim.html` (the virtual
device). Written after living in the simulator and reading the **measured** interaction
metrics it produces. The brief said be brutally honest, so this is critique, not
celebration — the things that work get one paragraph, the things that don't get the rest.

**One-line verdict:** the core *"look and ask"* loop is genuinely elegant and fast
(**1 action**); device autonomy is real and reassuring; but **everything that isn't
asking feels slow, fiddly, and timing-dependent**, first-run setup is not honestly
doable on-device, and the simulator's color/emoji flatter a V1 panel that will have
neither. Promising and useful at its core; frustrating at its edges; at risk of being
confusing without aggressive onboarding and phone-assisted setup.

---

## Measured metrics (from the simulator, wrap-aware optimal paths)

| Common task (from Home) | Min actions | Verdict |
|---|---|---|
| Ask AI about the view | **1** | excellent — the flagship is the fastest path |
| See next event | **0** | excellent — it's on the Home glance |
| Open full calendar | 2 | fine |
| Set an alarm | 2 | fine |
| Read a notification | 4 | mediocre |
| Change brightness | **4** | poor for a setting you'll touch often |
| Take a photo | **5** | poor for a camera-first device |
| View last photo | **6** | bad |

Navigation depth stays shallow (≤3), but **action *count* is the real cost** on a
one-button device, and the table shows the spread: the one thing the product is *for*
is 1 action; the everyday-but-secondary things cost 4–6. That asymmetry is the headline
finding.

---

## UX problems

1. **The simulator lies about color and icons.** It uses green/amber/red and emoji
   (📷🔔✦). A V1 256×64 **1-bit monochrome** OLED has *no color and no emoji* — warnings,
   errors, focus, and app identity all currently lean on cues the real panel cannot
   produce. Every meaning carried by color must be re-encoded as shape/position/blink/
   text before this is real. (Tracked across hardware tiers in FUTURE_HARDWARE_IMPACT.md.)
2. **Text quantity exceeds the panel.** AI answers, notification bodies, and calendar
   rows are written as if there's room. On 256×64 most of these truncate or scroll. The
   sim's comfortable two-line answers are optimistic.
3. **First-run setup is not honestly one-button.** The wizard hand-waves Wi-Fi password
   and API-key entry. Entering a Wi-Fi password by cycling characters with one button is
   *miserable* (dozens of actions, high error rate). **Conclusion: first-run must be
   phone-assisted** (the existing serial/phone provisioning path); the on-device wizard
   is at best a recovery fallback, and the simulator currently oversells it.
4. **Confirmation is invisible.** Actions (capture, dismiss, setting change) give a
   weak visual blip and nothing else. With no haptics/audio confirmation, a one-button
   user is never sure a press "took" — which causes double-presses, which the gesture
   grammar then misreads as a different action.

## Navigation problems

5. **The launcher is O(n) and won't scale.** Seven apps on a 1-D ring already cost up to
   3 presses to reach the far side. Add Translate, Maps, Music, Timer… and *every*
   secondary task gets worse linearly. There is no search, no recents, no favorites, no
   direct addressing — the model fights its own roadmap (PRODUCT_VISION V2/V3 add apps).
6. **"Where am I?" is unanswered.** On a tiny panel with shallow but real nesting
   (Gallery→item, Settings→value), there's no breadcrumb and BACK is an invisible
   long-press. Easy to feel lost; easy to long-press one beat too long and over-pop.
7. **Overlays gamble with attention.** Notification overlays *auto-dismiss* — glance
   away and you miss it; it's then 4 actions to retrieve. Meanwhile the low-battery
   *modal* seizes input. Two opposite failure modes (too transient / too blocking) from
   the same overlay system.

## Cognitive load

8. **PRIMARY is overloaded.** It means "ask the AI" on Home but "capture" in Camera. A
   single button whose meaning shifts by context is exactly the kind of mode-dependence
   that raises load on the input users rely on most.
9. **The gesture grammar is memorized, not discovered.** short=Select / long=Back /
   double=Next is invisible after the tutorial. New or returning users will mis-fire.
   There's no persistent affordance reminding them, and a tiny panel has no room for one.
10. **Dense Home glance.** Time + next event + unread + battery + AI-status crammed into
    one strip is a lot to parse at a glance on a small mono display — the opposite of the
    "glanceable" promise.

## One-button navigation weaknesses (the structural risk)

- **No direct selection** — everything is sequential cycling; cost grows with list length.
- **Timing-dependent** — long/double press introduce latency and ambiguity; fast users
  collide presses, slow users wait for the long-press threshold on *every* Back.
- **No undo / no escape hatch** beyond Back — an accidental Select into a deep screen
  costs a long-press to leave.
- **Text entry is effectively impossible** — hence setup must be offloaded (see #3).
- **It's a V1 necessity, not a virtue.** The design must treat one-button as a *floor*
  to escape (add encoder/touch/voice as input options), not a constraint to optimize
  around forever. The logical-action abstraction (UI_RUNTIME §5) already permits this —
  the UX just must not bake in one-button-only assumptions (it currently does, in the
  overloaded PRIMARY and the linear launcher).

## Features that feel unnecessary (for V1)

- **On-device Gallery browsing** (6 actions to view a photo). You're carrying a phone
  with a real screen — browsing 256×64 thumbnails is a worse version of something the
  phone does perfectly. Keep *capture*; defer *browse* to "open on phone."
- **Multiple clock faces** (analog/world) on a 1-bit panel — analog hands are hard to
  read tiny and mono; world clock is a power-user nicety competing for the most-used app.
- **Full Calendar app** beyond the Home glance — the glance ("next event") covers ~80%
  of the need; a scrollable agenda on a tiny panel is marginal value for its app slot.

## Features that feel missing

- **Phone-assisted setup & "open on phone" handoff** — the single biggest gap; the
  product *needs* the phone for text entry and for anything long-form, yet there's no
  handoff affordance.
- **A fast triage gesture for notifications** — dismiss/snooze without the 4-action trip.
- **Voice input** — the natural escape from one-button for asking and for text; the whole
  "ask" loop wants a mic more than a button.
- **Confirmation feedback** — haptic/audio tick on every committed action.
- **Recents / quick-launch** — a way to reach the last app in 1 action instead of N.
- **Ambient-aware brightness** — brightness is a 4-action manual chore today; it should
  rarely need touching.
- **An explicit "AI offline" pre-empt at ask time** — it's shown, but easy to miss; the
  device should make the degraded state unmissable *before* the user presses ask.

---

## Prioritized recommendations

1. **Move first-run to phone-assisted provisioning; demote the on-device wizard to
   recovery.** (Fixes the most dishonest part of the current flow.)
2. **Add a non-color visual language** (shape/position/blink/inverse) so nothing depends
   on color or emoji — prerequisite for the real panel.
3. **Add voice input and a confirmation tick** — together they relieve most one-button pain.
4. **Replace the linear launcher with recents-first + a short favorites set**; cap the
   on-device app count and push the long tail to the phone.
5. **Defer on-device Gallery browse and extra clock faces**; add an "open on phone" handoff.
6. **De-overload PRIMARY** or make its current meaning explicit on every screen.
7. **Rework overlays**: notifications persist a dismissible marker (no silent auto-loss);
   low-battery warns without a full modal until truly critical.

The good news: the core loop is right and the autonomy story is real and tested. The
work is almost entirely in the *secondary* navigation and in being honest about a
colorless, button-scarce V1 — both of which are cheaper to fix now, in the simulator,
than after the panel ships.
