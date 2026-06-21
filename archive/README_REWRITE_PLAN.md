# README_REWRITE_PLAN.md

> A review of the current `README.md` and a punch-list of what must change before the
> repository is shown publicly. **This file does not rewrite the README** — it is the
> spec for that rewrite. The canonical source of truth for any new wording is
> `PROJECT_BRIEF_SUDONIT.md` (founder vision wins over older docs).
>
> Why this matters: for an open project, the README is the *front door*. A
> journalist/contributor/investor clicks from the website to GitHub within minutes
> (`WEBSITE_REVIEW.md`). The current README would actively undercut the website.

---

## Verdict

The current README is **outdated and internally contradictory**, written for an earlier
"pre-prototype, display-someday" framing that the founder vision has since superseded.
It must be rewritten before launch. This is a **launch blocker** (`LAUNCH_READINESS.md`
§3 and blocker #5).

---

## Outdated information

1. **"Project Status: Pre-Prototype."** No longer true. The full capture→AI→audio loop
   runs on the host and is covered by an asserting interop test; hardware is arriving.
   "Pre-Prototype" undersells real, demonstrable work and contradicts a website that
   shows a working device.
2. **The 6-phase roadmap (Phase 0→6, ending in "Product Prototype").** Does not match the
   canonical **V1 / V2 / V3** framing in `PROJECT_BRIEF_SUDONIT.md`. Two different
   roadmaps in two front-facing places is a credibility problem.
3. **Display framed as a near-term goal** ("Phase 5 → Display Integration," "Display
   integration" under Future Goals). Canonically, **V1 is audio-first; the display is
   V2/experimental.** The README implies a nearer display than the project commits to.
4. **Architecture split implies "Smartphone: AI processing / Internet / Storage" as the
   only model.** Outdated vs. the canonical **three AI options** (your own key / hosted
   Sudonit AI / your own server). The current text reads as phone-only and omits the
   ownership-of-AI story that is central to the pitch.
5. **Feature lists mix shipping reality with distant aspiration** ("Real-time
   translation," "Navigation," "Open plugin ecosystem," "Custom applications") without
   separating *what runs today* from *long-term vision* — the exact "inverted
   vision-to-evidence ratio" flagged in `WEBSITE_REVIEW.md`.

---

## Contradictions (these are the most damaging)

1. **License says two different things.** The footer reads **"License: To be
   determined,"** but a full **Apache-2.0 `LICENSE` file** sits in the repo root. This is
   a direct, visible contradiction and a hard blocker — it makes the "open source" claim
   look unserious to the one audience (contributors) most likely to check.
2. **"Pre-Prototype" vs. a public website showing a working demo.** The README would
   call the project pre-prototype while the website shows it working. Pick the truth
   (it works, early prototype) and state it consistently everywhere.
3. **README architecture ("AI on the smartphone") vs. brief/website ("your key / hosted
   AI / your own server").** Contradicts the central ownership/privacy message.

---

## Missing sections (needed before public launch)

1. **Proof of life up top.** A real photo or the demo video/GIF near the top, and a link
   to it. The README currently has zero images of the device.
2. **"What actually works today"** — an honest status block: the capture→AI→audio loop
   runs, here are the passing tests (`tests/`), here's what's not built yet (no app,
   audio-first, no display in V1). This replaces "Pre-Prototype" with something both
   honest and impressive.
3. **A real license statement** — "Licensed under Apache-2.0" with a link to `LICENSE`,
   replacing "To be determined." (Confirm Apache-2.0 is the intended license; if so,
   just state it.)
4. **Build / run from a clean clone** — or a prominent link to `RUNNING.md`, verified to
   work. A contributor must be able to go from clone to running in minutes.
5. **The one-line "what is this"** sharpened to the canonical pitch — clip-on module that
   turns your *own* glasses smart; audio-first; you own the AI loop. (Current opening is
   close but generic.)
6. **Where to help / contribute** — link `CONTRIBUTING.md`, and ideally a few real
   "good first issues" and a community/contact channel (`WEBSITE_REVIEW.md`, contributor
   role: "contribute" with no repo link/license/issues feels hollow).
7. **The three AI options** stated briefly — own key / hosted / your own server — so the
   ownership story is visible at the front door.

---

## What can stay / is already fine

- The **core idea and philosophy** sections ("turn any glasses into smart glasses,"
  Open/Modular/Repairable/Customizable/Privacy) are on-message — keep, tighten, align
  wording with the brief.
- The **basic glasses/phone architecture** sketch is fine *once* the AI-options and
  audio-first corrections are folded in.
- The **contributing areas list** is fine; it just needs real links behind it.

---

## Rewrite punch-list (ordered, for when the rewrite happens)

1. **Fix the license line** → "Apache-2.0" + link. *(Hard blocker; do first, it's a
   one-line fix that removes a glaring contradiction.)*
2. **Replace "Pre-Prototype" + 6-phase roadmap** with an honest "what works today" block
   and the canonical V1/V2/V3 framing (audio-first V1, display V2).
3. **Add the three AI options** and correct the architecture section.
4. **Add proof:** a real photo / demo GIF near the top + link to the video.
5. **Add/verify build-and-run** (link `RUNNING.md`, confirm it works from a clean clone).
6. **Separate "today" from "vision"** in the feature lists.
7. **Wire up contribution path** (link `CONTRIBUTING.md`, license, issues, contact).
8. **Final pass for consistency** with `PROJECT_BRIEF_SUDONIT.md` and the live website —
   no claim in the README should contradict either.

**Reminder:** do not perform the rewrite from this file alone — pull exact wording from
`PROJECT_BRIEF_SUDONIT.md` so the README, brief, and website all say the same thing.
