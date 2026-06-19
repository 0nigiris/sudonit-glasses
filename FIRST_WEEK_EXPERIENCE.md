# FIRST_WEEK_EXPERIENCE.md — Red Team

Seven days with Sudonit V1, simulated honestly. To make this a *fair* stress test, the
user is the **most favorable realistic V1 owner**: "Sam," a 32-year-old software developer
who backed the idea, owns an API key, likes tinkering, and *wants it to succeed*. If even
Sam drifts away, a normal user never started.

Tracked daily: **excitement** and **frustration** (0–10), **actual usage** (real
interactions that day), and **abandonment risk**. V1 assumed: clip-on, audio-first (display
flaky/optional), phone-brokered Claude, Wi-Fi, ~1 h battery, one button.

---

## Day 1 — "This is the future" 🟢
- **Excitement 9 · Frustration 3 · Usage: ~15 asks.**
- Unboxes, clips it on, does phone-assisted setup (Wi-Fi + key). First "what is this?"
  works — a real answer in his ear in ~3 s. Shows three friends. Genuinely delighted.
- Friction already there but forgiven: the clip needs re-seating, the HUD (if fitted) is
  hard to read, one answer was wrong, battery died after ~1 h and he shrugged and charged it.
- **Abandonment risk: very low.** Novelty dominates.

## Day 2 — "Still cool, but…" 🟢
- **Excitement 8 · Frustration 4 · Usage: ~10 asks.**
- Uses it on a walk. Two great answers (a plant, a menu translation). But twice the answer
  took 6+ seconds and once it said "AI unavailable" (phone app had been killed in his
  pocket). He reopens the app, moves on.
- Notices he reaches for his phone anyway for anything that needs reading or follow-up.
- **Abandonment risk: low.** Still a toy he enjoys.

## Day 3 — The friction becomes visible 🟡
- **Excitement 6 · Frustration 6 · Usage: ~6 asks.**
- The pattern crystallizes: it only reliably works on his home Wi-Fi with the app open.
  Out and about, it's a coin flip. Latency makes him stop using it conversationally.
- Battery: he forgot to charge it; it was dead when he wanted it. The "second thing to
  charge" tax is now real.
- A coworker asked "are you recording me?" — mild but it lands.
- **Abandonment risk: rising.** The magic now has conditions.

## Day 4 — The tipping point 🟠
- **Excitement 4 · Frustration 7 · Usage: ~2 asks.**
- He wears it out of habit more than desire. Asks it one thing at lunch; 7-second wait,
  then a confidently wrong answer about a sign. He double-checks on his phone — which was
  faster and right anyway.
- The realization, stated to himself: *"Everything useful, my phone does better."*
- **This is the day Sam stops *caring*.** He still has it on, but he's no longer reaching
  for it; the phone won every contest today.
- **Abandonment risk: high.**

## Day 5 — Token effort 🟠
- **Excitement 3 · Frustration 5 · Usage: 1 ask.**
- Leaves it on the desk in the morning, grabs it on the way out as an afterthought. One
  ask, works fine, doesn't change his day. The clip annoyed the bridge of his nose.
- He's not angry — he's *indifferent*, which is worse.
- **Abandonment risk: high.**

## Day 6 — The drawer 🔴
- **Excitement 2 · Frustration 3 · Usage: 0.**
- Doesn't put it on. Doesn't notice he didn't. It's on the desk, uncharged.
- Frustration is *low* now only because he's stopped expecting anything from it.
- **Abandonment risk: near-certain.**

## Day 7 — Verdict 🔴
- **Excitement 2 · Frustration 2 · Usage: 0.**
- Sam's honest summary to the project: *"Incredibly cool project. I love that it's open. I
  stopped wearing it on day 4 because it was slower and less reliable than the phone
  already in my hand, and I had to babysit a second battery and an app to use it. If it
  were instant, all-day, and worked anywhere, I'd wear it every day."*
- **Final state: abandoned, with goodwill.** He'll star the repo and recommend the *idea*.
  He will not wear the *device*.

---

## The excitement/usage curve

```
 10 │●                         excitement
  8 │  ●                       ── declines steadily as conditions surface
  6 │     ●                    usage falls FASTER than excitement
  4 │        ●  ●              (he stops doing before he stops liking)
  2 │            ●  ●  ●
  0 └──┬──┬──┬──┬──┬──┬──┬──
     D1 D2 D3 D4 D5 D6 D7
 asks 15 10  6  2  1  0  0
```

## Exactly when the user stops caring

**Day 4, at the moment a 7-second wait produced a wrong answer and the phone was faster
and right.** That single comparison — *not* a crash, *not* a dead battery — is the kill
shot. The device didn't fail loudly; it lost a fair fight to the phone, and once a user
runs that comparison and the device loses, the relationship is over. Everything after Day 4
is inertia, not engagement.

## What would have moved the kill-point later (analysis, not new features)

The audit and PRODUCT_REVIEW already named these; the week localizes *why* they matter:
- **Reliability anywhere** (not just home Wi-Fi + app-open) would have prevented the Day 3
  "it has conditions" realization.
- **Sub-2-second, correct answers** would have prevented the Day 4 lost comparison — the
  single decisive event.
- **All-day battery** would have removed the Day 3/5 charging tax that made disuse easy.
- **A hands-free, glanceable thing the phone genuinely can't do** would have given a reason
  to win the comparison instead of losing it.

Absent those, the most favorable possible user abandons a flawless-code Sudonit in **four
days**. That is the number to design against.
