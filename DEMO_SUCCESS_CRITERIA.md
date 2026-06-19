# DEMO_SUCCESS_CRITERIA.md

The exact, measurable definition of **V1 success** — distinct from "it works." A demo that
*works* completes the loop once on a bench. A demo that is *successful* proves the device is
worth wearing. Most projects declare victory at "works" and that is why FIRST_WEEK_EXPERIENCE
ends in a drawer. This document raises the bar to the real line.

Criteria are grouped into three gates. **All three gates must pass.** Numbers reference
`DEMO_METRICS.md`; thresholds here are the *success* line, often stricter than "acceptable."

---

## Gate 1 — It functions on real hardware (necessary, not sufficient)

The loop runs on silicon, not the host. Pass = **all** true:

- [ ] Cold boot to running in **≤ 5 s**, 3/3 boots, no boot loop. *(DEMO_METRICS #1)*
- [ ] Wi-Fi associates in **≤ 8 s**, **≥ 9/10** attempts. *(#2)*
- [ ] Capture → intact JPEG on the phone (**SHA passes**) **≥ 19/20** captures. *(#3, demo step 4)*
- [ ] Full **capture → real Claude → answer → spoken audio** completes end-to-end on hardware.
- [ ] **No brownout/reset** during a camera+Wi-Fi+audio cycle, peak current recorded. *(#7)*

> This is the gate everyone *means* by "the demo worked." It is gate **1 of 3**.

## Gate 2 — It is good enough to be worth using (the real bar)

The four numbers that decide product-vs-project, at their *success* thresholds:

- [ ] **AI round-trip median ≤ 2.0 s and p90 ≤ 3.5 s.** *(#4)* — fast enough that the user
      doesn't lose the phone comparison.
- [ ] **Answer correctness ≥ 8/10** on the fixed test scenes, and the multimodal "AI
      agreement" check correct for **≥ 4/5** scenes. *(#9)* — right often enough to trust.
- [ ] **Audio answer begins ≤ 1 s** after the text is ready, intelligible, **no buzz** with
      the camera active. *(#5)*
- [ ] **Battery: ≥ 1 h continuous / ≥ a half-day realistic mixed use** on a wearable cell,
      enclosure **not uncomfortably warm**. *(#6–8)* — wearable, not desk-bound.
- [ ] **Error rate < 1 in 10** asks over a normal session (timeouts, refusals, drops). 

> Gate 2 is where "works" becomes "worth it." A device that passes Gate 1 but fails Gate 2
> is a successful *engineering* demo and a failed *product* demo.

## Gate 3 — A real person wants to keep using it (the verdict)

Bench numbers can't prove desire. One human, one week, unprompted:

- [ ] **A real user (ideally from the target community, not the builder) wears it for 7 days.**
- [ ] **Voluntary daily use:** they reach for it **unprompted ≥ 3 times/day on ≥ 5 of 7 days**
      — i.e. usage does **not** collapse after the novelty (the Day-4 cliff is not hit).
- [ ] **Sean-Ellis test:** asked *"how would you feel if you could no longer use it?"*, the
      answer is **"very disappointed."** This is the single most important criterion — it is
      the difference between a toy and a need.
- [ ] **They can name one thing it does better than their phone** and actually used it for
      that this week (hands-free / glanceable / always-there).
- [ ] **Net:** at end of week, the device is **on their face, not in a drawer.**

> Gate 3 is the criterion the project has never tested and the only one that predicts
> survival. It cannot be passed on a bench, by the builder, or in a day.

---

## The exact moment V1 is "successful"

> **V1 is successful the moment a real user from the target community, after seven days of
> voluntary use, says they would be "very disappointed" to lose it — backed by hardware that
> answers in under ~2 seconds, is right ~8 times in 10, doesn't brown out, and lasts long
> enough to wear.**

Not when the loop closes on the bench (Gate 1). Not when the numbers are green (Gate 2).
**When a person who isn't you would miss it (Gate 3).** Everything before that is progress;
only that is success.

## Anti-criteria (do NOT declare success on these)

- "The demo worked at the meetup." — Gate 1 only; a one-shot bench pass.
- "All host tests / CI green." — measures the easy half; says nothing about silicon or desire.
- "The simulator feels great." — it flatters hardware the device won't have.
- "It's architecturally elegant / well-documented." — true and irrelevant to whether anyone
  wears it.
- "I (the builder) use it." — the builder's tolerance is not the user's; doesn't count.

## If you must pick one number to chase first

**AI round-trip median latency (Gate 2)** — because the first-week simulation shows the user
is lost the moment a slow answer loses to the phone. Get that under 2 seconds and correct,
and Gate 3 becomes *possible*. Leave it at 6 seconds and no other success criterion can be
reached, no matter how good the rest is.
