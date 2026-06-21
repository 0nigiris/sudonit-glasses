# WHY_SUDONIT_LOSES.md — Postmortem (assume it failed)

Written from ~18 months in the future, assuming Sudonit shipped a working V1 and then
faded. All the code worked. It failed anyway. This is the honest postmortem.

---

## What "failure" looked like

Not a crash and not a scandal — a **quiet fade**. A working open-source repo with a few
hundred stars, a handful of makers who built one, a demo video with 20k views, and then:
commits slow, issues go unanswered, the maintainer moves on, the AliExpress board goes out
of stock, the provider SDK changes and the build breaks, and the project becomes a
beautifully documented archive of something that almost was. No users wear one a year later.

## Why it failed (in order of decisiveness)

1. **It had no market, only an audience.** The "open + your own API key" premise meant the
   only people who could and would use it were developers. That is an audience to admire the
   project, not a market to sustain it. Every other decision was downstream of this one.
2. **It lost a fair fight to the phone.** For its core flow it was slower, less reliable,
   and less accurate than the device already in everyone's pocket. Users ran that comparison
   in the first week (FIRST_WEEK_EXPERIENCE, Day 4) and the phone won.
3. **It tried to out-Ray-Ban Ray-Ban.** Against a vertically integrated, certified,
   good-looking, frictionless $300 product, "ours is open" was a values argument, and values
   arguments don't win daily wear from anyone but believers.
4. **The hard problems were deferred until they couldn't be.** Power, near-eye optics, a
   real phone app, and BLE were each "later." Reality arrived all at once: ~1 h battery, a
   HUD that wasn't focusable/readable, an app that the OS kept killing, and a power-hungry
   Wi-Fi link. None were solved because none were started early.
5. **Effort went where it was pleasant, not where it was risky.** Enormous, genuinely good
   design energy went into architecture, a UI runtime, eight apps, and a slick simulator —
   for a display that was never ordered and optics that never worked. The risky 20% (silicon,
   power, real AI, demand) got a fraction of the attention and held 100% of the risk.
6. **Solo bus factor.** One person carried vision, firmware, protocol, UX, hardware, and the
   app. When that person's energy dipped, there was no community to catch it, because there
   was never a runnable, reproducible thing easy enough for others to join.

## Which assumptions were wrong

- **"Host-green means de-risked."** It measured the easy half and hid the hard half.
- **"Battery is priority #2."** For a wearable, battery was priority #1 and treating it as
  secondary was fatal.
- **"Users will bring their own API key."** True of dozens of people, false of everyone else.
- **"A clip-on is simpler than a frame."** It was harder: balance, fit, optics alignment.
- **"The display is a parallel track that can catch up."** Optics were a research problem,
  not a sprint, and the whole UX was built on top of a part that never worked.
- **"Open + private is enough of a reason."** It was a reason to *respect* it, never a
  reason to *wear* it.

## What competitors did better

- **Meta (Ray-Ban):** vertical integration, real industrial design, certification,
  sub-3-second answers, no API key, in stores. Frictionless where Sudonit was fiddly.
- **Even Realities:** solved the one thing — a genuinely wearable, readable monochrome
  display — and shipped nothing it couldn't do well. Focus beat breadth.
- **Humane (instructive failure, the mirror):** also technology-led and also died — but its
  death was the *same* lesson Sudonit didn't internalize: putting the hard stuff (here,
  optics/power/market) on the wearable and hoping is how these projects end.

## Warning signs that were ignored

- The repo's **doc-to-running-code ratio kept rising** while *nothing ran on silicon.*
- A **mid-project architecture reversal** (dumb-client → autonomy-first) that *added* scope
  during a "fastest path to demo" phase — driven by philosophy, not evidence.
- The project's **own audit** named the killer question (no market) and the team kept
  building features-as-docs anyway.
- The **simulator flattered the hardware** (color/emoji/text the panel couldn't show), so
  UX confidence was partly synthetic.
- **The four numbers that mattered** — peak current, latency, answer-correctness, battery
  life — went unmeasured the longest, because they were the ones most likely to hurt.

## The one-sentence postmortem

> Sudonit failed not because the code was wrong but because it answered an engineering
> question ("can an open device do look-and-ask?") instead of a human one ("why would
> anyone wear this instead of using their phone?") — and by the time reality forced the
> human question, the energy, the battery, and the audience were already spent.
