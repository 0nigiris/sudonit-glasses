# VALIDATION.md

> Assumptions, risks, failure modes, and the evidence/metrics that turn beliefs into facts.
>
> Consolidated document — built by merging the sources below verbatim under
> labeled dividers (no information removed). See DOCS_CONSOLIDATION_PLAN.md.


---

<!-- ===== consolidated from: ASSUMPTION_REGISTER.md ===== -->

# ASSUMPTION_REGISTER.md — every belief the project rests on but has not proven

Sudonit has been designed, audited, attacked, and simulated. It has **never been measured
on hardware.** This register lists every significant assumption the project currently
depends on, scores how confident we are *honestly* (not how confident the docs sound), and
records the evidence on both sides.

**Rule applied throughout:** simulation, host tests, design elegance, and prior experience
are *not* evidence about this hardware. Confidence above ~60% requires evidence from a real
build, a real measurement, or a directly comparable shipped device. Most assumptions here
are below that line — that is the point of the document.

Columns per assumption: **Confidence** · **For** (evidence supporting) · **Against**
(evidence undermining) · **Status** (Unvalidated / Partially validated / Validated) · **How
to test** · **Cost to test** (time + money + hardware).

Statuses are deliberately harsh: anything not tested *on the actual hardware/users* is
**Unvalidated**, no matter how reasonable.

---

## A. Hardware & power

### A1. The board boots and runs our firmware at all
- **Confidence:** 80%
- **For:** Standard ESP32-S3-WROOM-1 N16R8, well-trodden ESP-IDF path; thousands of identical boards run.
- **Against:** Specific unit/seller unknown; sdkconfig untested on *this* board; no flash done yet.
- **Status:** Unvalidated.
- **How to test:** Flash blink + boot log, read PSRAM/flash detection.
- **Cost:** 30 min, $0 (board on hand).

### A2. Camera (OV5640) initializes and produces usable frames with our pin map
- **Confidence:** 45%
- **For:** OV5640 is common; driver skeleton written against a known pinout.
- **Against:** Pin map *assumed* (Freenove); `esp_camera_init` is notoriously pin/clock-sensitive; no capture has ever happened.
- **Status:** Unvalidated.
- **How to test:** Flash camera self-test, dump one JPEG over serial/Wi-Fi, view it.
- **Cost:** 1–2 h, $0.

### A3. PSRAM is detected and large enough for frame buffers
- **Confidence:** 70%
- **For:** N16R8 = 8 MB octal PSRAM; sdkconfig targets it.
- **Against:** Octal PSRAM config is fiddly; unverified on this unit.
- **Status:** Unvalidated.
- **How to test:** Boot log + `heap_caps` report.
- **Cost:** 15 min, $0.

### A4. The board does **not** brown out when camera + Wi-Fi spike together
- **Confidence:** 35%
- **For:** A good USB supply + bulk cap usually holds it.
- **Against:** This is the **#1 predicted day-one failure** (FAILURE_SCENARIOS #1); peak current unknown; on a small battery it is worse.
- **Status:** Unvalidated.
- **How to test:** USB current meter during a capture+upload+audio cycle; watch for resets.
- **Cost:** 1 h, ~$15 (inline USB power meter).

### A5. Battery life is *usable* (target ≥ 1 h continuous, ≥ half-day mixed)
- **Confidence:** 25%
- **For:** Duty-cycling and BLE could stretch it.
- **Against:** Wi-Fi + camera + compute on a wearable cell is brutal; WHY_LOSES and FAILURE #2 flag <1 h as likely; physics, not a bug.
- **Status:** Unvalidated.
- **How to test:** Measure average current in idle/active/peak; compute from a chosen cell; then run it down.
- **Cost:** Half-day + a cell/charger IC, ~$20–40.

### A6. The enclosure does not get uncomfortably warm against the temple
- **Confidence:** 40%
- **For:** Duty-cycling reduces sustained load.
- **Against:** Compute + radio in a tiny clip near skin; no thermal path designed; FAILURE #3.
- **Status:** Unvalidated.
- **How to test:** Thermocouple/IR on the enclosure during sustained use; wear test.
- **Cost:** 1 h, ~$15 (IR thermometer).

### A7. I2S audio pins (40/41/42) are broken out and the amp produces clean audio
- **Confidence:** 55%
- **For:** Driver skeleton written; pins chosen from a header map.
- **Against:** Breakout unverified; PCLK→I2S buzz with camera active is a known risk (FAILURE #8).
- **Status:** Unvalidated.
- **How to test:** Play a tone, then play with the camera streaming; listen + scope.
- **Cost:** 1–2 h, $0 (amp on hand).

### A8. The clip-on form factor mounts to real glasses without damaging them or falling off
- **Confidence:** 30%
- **For:** Simple clips exist.
- **Against:** "Fits every frame" is an unbounded mechanical problem (WHY_LOSES); balance, fit, alignment all unsolved; nothing fabricated.
- **Status:** Unvalidated.
- **How to test:** 3D-print one bracket, mount on 3 different frames, wear for an hour.
- **Cost:** Days (CAD + print iterations), ~$0–30 filament.

## B. AI, network & latency

### B9. Claude reliably understands SVGA/low-res, real-world phone-captured frames
- **Confidence:** 50%
- **For:** Modern multimodal models are strong on clear images; our test scenes will be curated.
- **Against:** Real frames are backlit/motion-blurred/low-res; **confidently wrong answers are the most insidious failure** (FAILURE #18); never tested on *our* camera's output.
- **Status:** Unvalidated.
- **How to test:** Send 20 real captured frames, score answer correctness blind (DEMO_METRICS #9).
- **Cost:** 2–3 h once camera works, ~$1 API.

### B10. AI round-trip latency is fast enough to feel good (median ≤ 2 s, p90 ≤ 3.5 s)
- **Confidence:** 30%
- **For:** Cloud multimodal calls *can* be ~1–2 s.
- **Against:** Capture + upload over Wi-Fi + model + TTS + downlink is many hops; FIRST_WEEK Day-4 kill-shot is a 7 s answer; the single most decisive number, unmeasured.
- **Status:** Unvalidated.
- **How to test:** Instrument each hop end-to-end on hardware; record median/p90 (DEMO_METRICS #4).
- **Cost:** 2 h once loop closes, ~$1 API.

### B11. The Wi-Fi data plane works in the real world, not just home Wi-Fi
- **Confidence:** 30%
- **For:** Wi-Fi stack is implemented and works on a known network in principle.
- **Against:** 5 GHz-only, captive portals, enterprise 802.1X, hidden SSIDs all break it; can't type a portal login on one button (FAILURE #11).
- **Status:** Unvalidated (host-side code exists; never run against hostile networks).
- **How to test:** Try to connect/use it at 5 venues (cafe, office, phone hotspot, enterprise, home).
- **Cost:** A day of field testing, $0.

### B12. The phone broker stays alive in the user's pocket (app not killed by the OS)
- **Confidence:** 25%
- **For:** A proper foreground service *can* persist.
- **Against:** No phone app exists yet; iOS/Android background limits routinely kill brokers (FAILURE #12); "AI silently stops" is a trust-killer.
- **Status:** Unvalidated (the app is 0% built).
- **How to test:** Build a minimal companion, background it, measure how long the link survives.
- **Cost:** Weeks (a real app is a project), $0–99 (dev accounts).

### B13. The BLE control plane can be built and is adequate (it is currently 0% built)
- **Confidence:** 40%
- **For:** ESP32-S3 has BLE; the design is sketched.
- **Against:** None of it exists; pairing/reconnect/throughput unproven; a known hard part deferred.
- **Status:** Unvalidated / not started.
- **How to test:** Implement minimal BLE link, measure pairing reliability + throughput.
- **Cost:** 1–2 weeks, $0.

## C. Optics & display

### C14. A flat OLED + beam-splitter at ~2 cm produces a focusable, readable near-eye image
- **Confidence:** 15%
- **For:** Beam-splitter HUDs exist in products.
- **Against:** The eye cannot focus at 2 cm without collimation optics; **the core optics risk** (FAILURE #21); no display ordered; this underpins the entire UI runtime.
- **Status:** Unvalidated (no optics on hand).
- **How to test:** Bench a panel + splitter (or buy a known-good HUD module) and try to read it.
- **Cost:** Days–weeks + ~$30–150 parts, possibly a dead end.

### C15. The HUD is visible in daylight
- **Confidence:** 20%
- **For:** Bright sources exist.
- **Against:** Cheap mono panels wash out outdoors (FAILURE #22); brighter = more power (conflicts with A5).
- **Status:** Unvalidated.
- **How to test:** Read the HUD outdoors at noon.
- **Cost:** Minutes once a display exists; gated on C14.

### C16. Monochrome ~256×64 text is enough to be useful near-eye
- **Confidence:** 35%
- **For:** Short glanceable lines can work (Even Realities ships this).
- **Against:** AI answers truncate; tiny text unreadable; the simulator *lied* with color/emoji it can't show (DESIGN_REVIEW, FAILURE #27).
- **Status:** Unvalidated.
- **How to test:** Render real answers on the real panel; have someone read them.
- **Cost:** Hours once C14 passes.

## D. Interaction & UX

### D17. One-button navigation is not frustrating across 8 apps
- **Confidence:** 30%
- **For:** Minimal grammar (short/long/double) is learnable; simulator validated the state machine.
- **Against:** Users forget the grammar and get lost (FAILURE #23/#29); simulator measured Take-photo = 5 actions, View-photo = 6; the host sim can't feel the frustration.
- **Status:** Partially validated *in simulation only* (logic correct ≠ ergonomically OK).
- **How to test:** Hand the device to 3 people cold; count getting-lost events and time-to-task.
- **Cost:** A day of user testing once hardware exists.

### D18. First-time setup can actually be completed
- **Confidence:** 55%
- **For:** Decision already made: phone-assisted setup (Wi-Fi + key on phone in ~30 s).
- **Against:** The phone app to do it doesn't exist (see B12); on-device entry would be a trap (FAILURE #24).
- **Status:** Unvalidated (depends on an unbuilt app).
- **How to test:** Watch 3 first-time users complete setup unaided.
- **Cost:** Gated on the companion app.

### D19. Users tolerate Bring-Your-Own API key
- **Confidence:** 20%
- **For:** Developers/makers will (a few hundred).
- **Against:** The audit's **killer question** — BYO-key gives an *audience, not a market* (PROJECT_AUDIT, WHY_LOSES #1); normal users won't get/manage a key or a bill (FAILURE #15).
- **Status:** Unvalidated (and strongly doubted).
- **How to test:** Ask 10 non-developers to set up an API key; measure drop-off; or interview the target community on willingness.
- **Cost:** A week of customer discovery, $0 — **the cheapest high-value test in the whole project.**

### D20. There is a real user who would be "very disappointed" to lose it (Sean-Ellis)
- **Confidence:** 20%
- **For:** The assistive/hands-free niche has a genuine unmet need (WHY_WINS).
- **Against:** FIRST_WEEK shows even the friendliest user abandons in 4 days; never put in front of a target-community user.
- **Status:** Unvalidated — **the single most important untested belief** (DEMO_SUCCESS_CRITERIA Gate 3).
- **How to test:** 7-day wear test with one target-community user; run the Sean-Ellis question.
- **Cost:** Gated on a usable device; the verdict that decides everything.

### D21. The device does at least one thing genuinely better than the phone
- **Confidence:** 25%
- **For:** Hands-free + glanceable + always-there is real for specific jobs.
- **Against:** "Worse than the phone in my pocket" is High-likelihood (FAILURE #30); the phone won every contest in FIRST_WEEK.
- **Status:** Unvalidated.
- **How to test:** Define the one job; A/B it against the phone with real users; measure who wins.
- **Cost:** A few days of comparative testing.

### D22. Social acceptability — people will actually wear a face-camera clip in public
- **Confidence:** 25%
- **For:** Ray-Ban Meta normalized some of this.
- **Against:** A bare camera-clip draws odd looks; "are you recording me?" (FAILURE #25/#26); stays home.
- **Status:** Unvalidated.
- **How to test:** Wear a mockup (even non-functional) in 3 public settings; note reactions and own comfort.
- **Cost:** Hours, ~$0 — can be tested *today* with a dummy.

## E. Project & sustainability

### E23. "Host-green / CI-green" meaningfully de-risks the product
- **Confidence:** 30%
- **For:** It does catch logic regressions and keeps the build reproducible.
- **Against:** It measures the *easy half*; WHY_LOSES names "host-green means de-risked" as a wrong assumption that hid 100% of the real risk.
- **Status:** Partially validated (for what it covers) / **misleading** for product risk.
- **How to test:** Compare host-passing behavior against first on-silicon behavior; count divergences.
- **Cost:** Free once hardware runs.

### E24. The simulator faithfully predicts the real UX
- **Confidence:** 25%
- **For:** It validated the state machine and navigation logic (16/16 assertions).
- **Against:** It **flatters hardware** — color, emoji, instant response, readable text the 1-bit panel won't have (DESIGN_REVIEW, FAILURE/LOSES both flag this).
- **Status:** Partially validated (logic only); **not** validated for look/feel/latency.
- **How to test:** Put the same flow on hardware and diff the experience.
- **Cost:** Free once hardware runs.

### E25. A community will form and contribute (curing the solo bus factor)
- **Confidence:** 25%
- **For:** Open repo, good docs.
- **Against:** Nothing runnable on hardware to attract contributors; doc-to-code ratio rising; maintainer-burnout risk is Med-High (FAILURE #33, WHY_LOSES #6).
- **Status:** Unvalidated.
- **How to test:** Ship one reproducible on-hardware demo + good-first-issues; measure outside contributions over a quarter.
- **Cost:** Ongoing; gated on a runnable artifact.

### E26. Firmware can be updated safely in the field (no bricking)
- **Confidence:** 30%
- **For:** ESP-IDF supports A/B OTA.
- **Against:** Not built; a bad flash = dead unit for non-makers (FAILURE #32).
- **Status:** Unvalidated / not started.
- **How to test:** Implement A/B OTA + serial recovery; deliberately push a bad image and recover.
- **Cost:** ~1 week.

---

## The shape of this register

Count the confidences. The things we are *confident* about (A1, A3) are trivial and cheap.
The things the **entire product thesis** depends on — focusable optics (C14, 15%), usable
latency (B10, 30%), real battery (A5, 25%), a market for BYO-key (D19, 20%), a user who'd
miss it (D20, 20%) — are exactly the ones with the **lowest confidence and zero validation.**

> The project's confidence is inversely correlated with each assumption's importance. We are
> sure about what doesn't matter and guessing about what decides survival.

The cheapest, highest-value tests (D19 customer discovery, D22 wear a dummy in public, A4
current meter, B11 field Wi-Fi) need little or no working hardware and should start
**immediately.** See `VALIDATION_BACKLOG.md` for the ranked plan and `TRUTH_TABLE.md` for the
brutal known/unknown split.


---

<!-- ===== consolidated from: VALIDATION_BACKLOG.md ===== -->

# VALIDATION_BACKLOG.md — ranked experiments to turn assumptions into facts

Derived from `ASSUMPTION_REGISTER.md`. Every item is an **experiment that produces evidence**,
not a feature. The list is sorted by the only ranking that matters before hardware:

> **highest uncertainty × lowest cost first.**

An experiment that kills a low-confidence, high-importance belief for an hour of work is worth
more than a month of building on top of that belief. Several top items need **no working
device at all** — they are about the market and the human, and they are the cheapest evidence
in the entire project.

Each experiment: **Goal · Hardware · Software · Time · Expected outcome · Decision enabled.**
"Decision enabled" is the reason to run it — if a result wouldn't change a decision, it isn't
on the list.

Ranking key: **uncertainty** (from the register's confidence — lower confidence = higher
uncertainty) and **cost** (time + money + hardware dependency).

---

## Tier 0 — Do now. No working hardware required. Highest leverage in the project.

### V1. Market reality: does anyone beyond developers want this? *(assumption D19, conf 20%)*
- **Goal:** Find out whether BYO-key + clip-on glasses answer a real demand, or only an audience.
- **Hardware:** None.
- **Software:** None. A list of 10–15 target-community people (assistive users, language learners, makers) and an interview script.
- **Time:** ~1 week of conversations.
- **Expected outcome:** Most say "interesting" but won't manage a key/bill; a small segment with a hands-free need lights up. (Either result is decisive.)
- **Decision enabled:** Whether to keep the mainstream framing at all, or pivot to the assistive/maker niche (WHY_WINS) — **the most important decision the project faces, testable for $0.**

### V2. Social acceptability with a dummy *(D22, conf 25%)*
- **Goal:** Learn if a face-camera clip is wearable in public before building one.
- **Hardware:** A non-functional 3D-printed/cardboard mockup on real glasses.
- **Software:** None.
- **Time:** A few hours over 2–3 days.
- **Expected outcome:** Noticeable looks; own self-consciousness measurable; informs discreet vs. assistive framing.
- **Decision enabled:** Form-factor and target-context (public vs. assistive/home) before any mechanical work.

### V3. Latency budget on paper + cloud-only spike *(B10, conf 30%)*
- **Goal:** Estimate the end-to-end answer time from a phone (no glasses) to see if ≤ 2 s is even physically reachable.
- **Hardware:** A phone + your API key.
- **Software:** A 30-line script: capture a photo, send to Claude, TTS, measure each hop.
- **Time:** Half a day.
- **Expected outcome:** A realistic floor for round-trip time *without* the ESP32/Wi-Fi hops; if the phone alone is already ~3 s, the glasses cannot beat it.
- **Decision enabled:** Whether the core flow can ever win the phone comparison (FIRST_WEEK Day-4); whether to invest in glasses latency at all.

### V4. Optics feasibility on a bench, cheaply *(C14, conf 15% — highest uncertainty in the project)*
- **Goal:** Find out if *any* affordable near-eye scheme is focusable/readable before the UI runtime assumes it.
- **Hardware:** A cheap OLED + beam-splitter **or** a known-good off-the-shelf HUD module (~$30–150).
- **Software:** Display a static test pattern.
- **Time:** A few days incl. shipping.
- **Expected outcome:** Likely "flat panel at 2 cm can't be focused"; possibly "the bought module works." Either ends months of guessing.
- **Decision enabled:** Whether V1 has a display at all, or ships **audio-first** and treats optics as research. Unblocks/kills C15, C16, D17-display, and the entire UI-runtime investment.

## Tier 1 — Do as soon as the board is powered. Cheap, fast, decisive.

### V5. Boot + PSRAM + flash sanity *(A1/A3, conf 80%/70%)*
- **Goal:** Confirm the board runs our config.
- **Hardware:** The board + USB.
- **Software:** Blink + boot-log dump (exists).
- **Time:** 30 min.
- **Expected outcome:** Boots, PSRAM detected. (If not, everything stops here — so test first.)
- **Decision enabled:** Green-light all further bring-up.

### V6. Brownout / peak-current measurement *(A4, conf 35%)*
- **Goal:** Measure peak current and confirm the board survives camera + Wi-Fi + audio together.
- **Hardware:** Board + **inline USB power meter (~$15)**.
- **Software:** A cycle that fires camera, Wi-Fi, and audio.
- **Time:** 1 h.
- **Expected outcome:** A peak-current number and a pass/fail on resets — the **#1 predicted day-one failure** confirmed or cleared.
- **Decision enabled:** Supply/cap design; whether a wearable battery is even viable (feeds A5).

### V7. Camera first light *(A2, conf 45%)*
- **Goal:** Get one real, intact JPEG out of the OV5640 with our pin map.
- **Hardware:** Board + camera.
- **Software:** Camera self-test + dump over serial/Wi-Fi (skeleton exists).
- **Time:** 1–2 h.
- **Expected outcome:** Either a usable frame or a pin/clock fix. Unblocks every AI experiment.
- **Decision enabled:** Pin map correctness; image-quality baseline; gates B9.

### V8. Audio out + camera-active buzz check *(A7, conf 55%)*
- **Goal:** Confirm clean audio, including with the camera streaming.
- **Hardware:** Board + I2S amp/speaker.
- **Software:** Tone + speech playback (skeleton exists).
- **Time:** 1–2 h.
- **Expected outcome:** Clean tone, or a documented buzz to mitigate (FAILURE #8).
- **Decision enabled:** Whether audio-first V1 (the likely path if optics fail) is viable.

## Tier 2 — Once camera + network are alive. The four numbers that decide product-vs-project.

### V9. AI answer correctness on real frames *(B9, conf 50%)*
- **Goal:** Score how often Claude is right on *our camera's* real-world output.
- **Hardware:** Working camera + phone/API.
- **Software:** Capture→send→log; blind scoring sheet (DEMO_METRICS #9).
- **Time:** 2–3 h, ~$1 API.
- **Expected outcome:** A correctness fraction on 20 real scenes; exposes the silent **confidently-wrong** failure (#18).
- **Decision enabled:** Trustworthiness; resolution/capture-feedback needs.

### V10. End-to-end latency on hardware *(B10, conf 30%)*
- **Goal:** Measure real median/p90 round-trip on the device (not the V3 phone floor).
- **Hardware:** Full loop.
- **Software:** Per-hop instrumentation (DEMO_METRICS #4).
- **Time:** 2 h, ~$1.
- **Expected outcome:** The decisive number; ≤ 2 s = a chance, ~6 s = FIRST_WEEK Day-4.
- **Decision enabled:** Ship/iterate/kill on the single most important metric.

### V11. Battery rundown on a real cell *(A5, conf 25%)*
- **Goal:** Convert measured current into real wear-time.
- **Hardware:** Board + chosen LiPo + charger IC (~$20–40).
- **Software:** Idle/active/peak current logging (DEMO_METRICS #6/#8).
- **Time:** Half a day.
- **Expected outcome:** Hours of continuous + mixed-use runtime; likely the harsh "<1 h" reality.
- **Decision enabled:** Wearable vs. desk-bound framing; duty-cycle/BLE priority.

### V12. Field Wi-Fi reality *(B11, conf 30%)*
- **Goal:** See how often the data plane works away from home.
- **Hardware:** Working board.
- **Software:** Existing Wi-Fi path + a connect/use probe.
- **Time:** A day across 5 venues.
- **Expected outcome:** "Works on known networks only" likely confirmed (FAILURE #11).
- **Decision enabled:** Whether BLE/phone-tether becomes mandatory for V1.

## Tier 3 — Needs an app and/or a usable device. Expensive, but the verdict lives here.

### V13. Thermal wear test *(A6, conf 40%)* — IR thermometer (~$15), 1 h, once V6/V11 run.
- **Decision enabled:** Wearability; duty-cycle limits.

### V14. One-button usability cold test *(D17, conf 30%)* — 3 naive users, a day, once a device exists.
- **Decision enabled:** Cut apps / change grammar / add voice.

### V15. Companion-app persistence *(B12, conf 25%)* — weeks (a real app); measure how long the broker survives backgrounding.
- **Decision enabled:** Whether phone-brokered AI is viable at all, or BLE-direct is required.

### V16. The verdict: 7-day wear + Sean-Ellis *(D20, conf 20% — the most important untested belief)*
- **Hardware:** A device good enough to wear (gated on V6–V12 passing).
- **Software:** Whatever it takes to make the loop usable for a week.
- **Time:** 7 days + recruiting one target-community user.
- **Expected outcome:** "Very disappointed" to lose it = success; or the Day-4 cliff again.
- **Decision enabled:** Whether V1 is a product or a project (DEMO_SUCCESS_CRITERIA Gate 3). **Everything else exists to make this test possible and honest.**

---

## The ranking, in one line each (uncertainty × cheapness)

| # | Experiment | Conf | Cost | Why it's ranked here |
|---|-----------|------|------|----------------------|
| V1 | Market/BYO-key discovery | 20% | $0, 1 wk | Lowest confidence on the thesis, costs nothing, gates the pivot |
| V2 | Dummy in public | 25% | $0, hrs | Form-factor go/no-go, today, no hardware |
| V3 | Phone-only latency floor | 30% | ½ day | Tells you if the core flow can *ever* win, no glasses |
| V4 | Optics bench | 15% | days, ~$100 | Highest uncertainty in the project; kills/keeps the whole display track |
| V5 | Boot/PSRAM | 80% | 30 min | Trivial but blocks everything; do first on power-up |
| V6 | Brownout/current | 35% | 1 h, $15 | #1 day-one failure; cheap meter |
| V7 | Camera first light | 45% | 1–2 h | Unblocks all AI tests |
| V8 | Audio + buzz | 55% | 1–2 h | Enables audio-first fallback |
| V9 | Answer correctness | 50% | 2–3 h, $1 | Trust; the silent killer |
| V10 | Latency on hardware | 30% | 2 h, $1 | The decisive number |
| V11 | Battery rundown | 25% | ½ day, ~$30 | Wearable-or-not |
| V12 | Field Wi-Fi | 30% | 1 day | Real-world reliability |
| V13 | Thermal | 40% | 1 h, $15 | Wearability |
| V14 | One-button cold test | 30% | 1 day | UX truth vs. simulator |
| V15 | App persistence | 25% | weeks | Viability of phone-brokering |
| V16 | 7-day + Sean-Ellis | 20% | 7 days | The verdict; everything serves this |

> **The four Tier-0 experiments (V1–V4) cost almost nothing, need almost no hardware, and
> attack the four lowest-confidence beliefs in the project. They should be running before the
> first line of firmware is flashed.** Doing them late is the exact mistake WHY_LOSES describes:
> the risky 20% measured last.


---

<!-- ===== consolidated from: TRUTH_TABLE.md ===== -->

# TRUTH_TABLE.md — what we actually know vs. what we believe

One table, three buckets, every major project belief sorted into exactly one. The rule is
ruthless and non-negotiable:

> **If there is no evidence from this hardware or a real user, it is Unknown — no matter how
> reasonable, well-designed, or well-documented it is.**

Design elegance is not truth. Host tests are not truth about silicon. Simulation is not truth
about hardware or humans. Prior experience with *other* boards is not truth about *this* one.
Under this rule almost everything is **Unknown**, and that is the honest state of the project.

Each row notes the source of its placement. Cross-references: `ASSUMPTION_REGISTER.md` (Axx),
`FAILURE_SCENARIOS.md` (#n), `DEMO_METRICS.md`.

---

## KNOWN TRUE — evidence exists (mostly about software/process, not the product)

| Belief | Evidence (why it's here) |
|--------|--------------------------|
| The host build compiles and host tests pass (16/16 sim assertions, net self-test). | Ran locally; CI green. *Scope: the host, not the device.* |
| The UI state machine's **logic** is internally consistent and composes (orthogonal regions). | Validated headless in the simulator. *Logic only — not look/feel/latency.* |
| The wire protocol is well-defined (length-prefixed frames, SHA-verified chunks). | Specified and exercised host-side. |
| ESP32-S3-WROOM-1 N16R8 + OV5640 are real, available, mainstream parts. | Datasheets; boards in hand; thousands shipped elsewhere. |
| Modern multimodal models *can* answer questions about *clear* images. | Public, widely reproduced — **but not on our camera's frames** (that's Unknown, B9). |
| Ray-Ban Meta / Even Realities / Humane outcomes (what shipped, what sold, what died). | Public market record; basis of COMPETITOR_ANALYSIS and the postmortem. |
| The repo is well-documented and the architecture is coherent on paper. | Self-evident from the docs. *Coherent ≠ correct ≠ wanted.* |

**Notice:** almost everything Known True is about *our software process* or *other people's
hardware/markets.* Almost nothing here is a measured fact about *the Sudonit device.*

## KNOWN FALSE — we have evidence these are not true (or not yet true)

| Belief | Evidence (why it's here) |
|--------|--------------------------|
| "The phone companion app exists / works." | It is **0% built** (B12). |
| "The BLE control plane exists." | **0% built** (B13). |
| "Safe field firmware update (A/B OTA + recovery) exists." | **Not started** (E26, FAILURE #32). |
| "A display has been ordered / bench-tested." | None ordered; optics untouched (C14). |
| "Anything has run on the actual silicon." | No flash has happened yet — the foundational fact of this whole exercise. |
| "Host-green means the product is de-risked." | WHY_LOSES identifies this as a *wrong* assumption: it measures the easy half. |
| "The simulator represents the real look/feel." | DESIGN_REVIEW: it shows color/emoji/instant text the 1-bit panel **cannot**. |
| "A real end user has tried it." | Never happened; the most important test (D20) is untouched. |

## UNKNOWN — no evidence either way. This is where the product lives or dies.

Everything that decides whether Sudonit is a product is here. Each is currently a *guess*.

### Hardware / power
- Whether the board **browns out** on camera+Wi-Fi spike (A4, #1) — *the #1 predicted failure.*
- **Peak and average current**; therefore **battery life** (A5, #2) — usable or <1 h? Unknown.
- **Thermal** against the temple (A6, #3).
- Whether the **camera pin map** is right and frames are usable (A2, #5).
- Whether **I2S audio** is clean with the camera active (A7, #8).
- Whether the **clip** mounts safely to real frames without damage (A8, #10).

### AI / network / latency
- Whether Claude is **reliably right on our real frames** (B9, #18) — or confidently wrong.
- **End-to-end round-trip latency** (B10, #4) — *the single most decisive number, unmeasured.*
- Whether **Wi-Fi works in the wild**, not just home (B11, #11).
- Whether the **phone broker survives** backgrounding (B12, #12).

### Optics / display
- Whether a near-eye image is **focusable** at all (C14, #21) — *highest-uncertainty belief, 15%.*
- Whether the HUD is **visible in daylight** (C15, #22).
- Whether **mono ~256×64 text** is useful (C16, #27).

### Interaction / UX / market
- Whether **one-button nav** frustrates real users (D17, #23).
- Whether **setup can be completed** by a normal person (D18, #24).
- Whether anyone **tolerates BYO-key** beyond developers (D19) — *the audit's killer question.*
- Whether a real user would be **"very disappointed" to lose it** (D20) — *the verdict.*
- Whether the device does **one thing better than the phone** (D21, #30).
- Whether people **wear a face-camera clip in public** (D22, #26).

### Project / sustainability
- Whether a **community forms** to cure the bus factor (E25, #33).
- Whether the **simulator predicted the real UX** (E24) — testable only on hardware.

---

## The brutal count

- **Known True:** ~7 — and nearly all about *software process* or *other companies*, not the device.
- **Known False / not-yet-built:** ~8 — the hard parts honestly admitted as absent.
- **Unknown:** ~22 — **and every single belief that determines whether Sudonit is a product
  is in this column.**

> We know our code compiles and that other people's glasses sold or died. We do **not** know
> whether ours boots without browning out, lasts an hour, can be read, answers fast enough,
> is right often enough, or whether one human on earth would miss it. The entire product is
> currently **Unknown**, and pretending otherwise — because the docs are thorough and the
> sim is slick — is the precise failure mode the postmortem warns about.

The job from here is mechanical and unglamorous: move rows out of **Unknown** by collecting
evidence, cheapest-and-most-uncertain first (`VALIDATION_BACKLOG.md`). Nothing should be
treated as true until it has moved columns *with evidence attached.*


---

<!-- ===== consolidated from: RISKS.md ===== -->

# RISKS.md

# Purpose

This document tracks technical, business and development risks.

A known risk is easier to manage than an unknown risk.

---

# Risk Levels

LOW

MEDIUM

HIGH

CRITICAL

---

# Hardware Risks

## Display Integration

Risk Level:

CRITICAL

Description:

Suitable display may be difficult to source.

Potential Problems:

* Cost
* Brightness
* Driver support
* Availability

Mitigation:

* Test multiple display options
* Delay final display selection until prototype exists

Status:

OPEN

---

## Optics

Risk Level:

CRITICAL

Description:

Display is useless without proper optics.

Potential Problems:

* Alignment
* Eye comfort
* Image quality

Mitigation:

* Research prism systems
* Research birdbath optics

Status:

OPEN

---

## Battery Placement

Risk Level:

MEDIUM

Description:

Weight distribution may be uncomfortable.

Potential Problems:

* Head fatigue
* Imbalance

Mitigation:

* Dual-arm battery layout

Status:

OPEN

---

## Audio Quality

Risk Level:

LOW

Description:

Open-ear speakers may not meet expectations.

Mitigation:

* Test multiple speaker designs

Status:

OPEN

---

# Software Risks

## Bluetooth Reliability

Risk Level:

HIGH

Description:

Unstable communication between phone and glasses.

Mitigation:

* Define protocol early
* Build simulator

Status:

OPEN

---

## AI Costs

Risk Level:

HIGH

Description:

Cloud inference costs may become expensive.

Mitigation:

* Multiple providers
* Compression
* Efficient prompts

Status:

OPEN

---

## Technical Debt

Risk Level:

HIGH

Description:

Fast prototyping may create long-term maintenance problems.

Mitigation:

* Documentation
* Architecture reviews

Status:

OPEN

---

# Product Risks

## User Comfort

Risk Level:

CRITICAL

Description:

Uncomfortable glasses will fail regardless of features.

Mitigation:

* Minimize weight
* Balance components

Status:

OPEN

---

## Complexity

Risk Level:

HIGH

Description:

Too many features may delay product development.

Mitigation:

* Follow roadmap
* Avoid scope creep

Status:

OPEN

---

# Founder Risks

## Burnout

Risk Level:

HIGH

Description:

Project may become overwhelming.

Mitigation:

* Focus on one phase at a time
* Track progress

Status:

OPEN

---

# Rule

Every major problem discovered during development should be added here.


---

<!-- ===== consolidated from: FAILURE_SCENARIOS.md ===== -->

# FAILURE_SCENARIOS.md — Red Team

Assume hardware arrives and all code compiles. These are the fastest realistic ways
Sudonit still fails. 35 scenarios across power, network/AI, optics/UX, ergonomics,
cost, and maintenance. Each: **what happens · likelihood · detectability · recoverability
· mitigation.** Likelihood and the rest are judged for V1 as designed (clip-on, ESP32-S3,
OV5640, Wi-Fi, phone-brokered AI, BYO key).

Likelihood: Low / Med / High / Near-certain.

---

## Power & hardware

**1. Brownout on camera + Wi-Fi power-up.** Board resets the instant both heavy loads
spike together. *Likelihood:* High. *Detect:* obvious (reset/reboot loop, visible on
serial + USB meter). *Recover:* easy if caught (USB supply, bulk cap, sequence loads).
*Mitigation:* quality supply, bulk capacitance, bring subsystems up one at a time; this
is the #1 predicted day-one failure.

**2. Battery lasts < 30 min.** Untethered runtime is a fraction of usable. *Likelihood:*
High. *Detect:* easy (DEMO_METRICS #8). *Recover:* hard — physics, not a bug. *Mitigation:*
bigger cell (weight ↑), duty-cycle, BLE instead of Wi-Fi, lower resolution; may force
"tethered/desk" framing for V1.

**3. Thermal — warm against the temple.** Compute + radio heat the enclosure on skin.
*Likelihood:* Med-High. *Detect:* easy (touch + thermometer). *Recover:* hard in a small
clip. *Mitigation:* thermal break, duty-cycle, move heat outward; measure before wearing.

**4. LiPo swelling / safety event.** A cheap unprotected cell behind the ear vents or
swells. *Likelihood:* Low but Critical impact. *Detect:* late (until it's bad).
*Recover:* none after the fact. *Mitigation:* protected cells, proper charger IC, never
ship a kit with a bare LiPo near a face.

**5. Camera pin map wrong.** `esp_camera_init` fails or returns garbage on first flash.
*Likelihood:* Med. *Detect:* easy (init error / unusable frames). *Recover:* easy
(remap from seller diagram). *Mitigation:* verify pins before the first camera flash.

**6. PSRAM not detected.** No frame buffer → camera dead. *Likelihood:* Low-Med. *Detect:*
easy (boot log). *Recover:* config fix or it's a bad unit. *Mitigation:* confirm in boot
log; sdkconfig already targets octal N16R8.

**7. I2S audio pins not broken out.** Chosen GPIO 40/41/42 aren't on the headers.
*Likelihood:* Low-Med. *Detect:* easy (no output). *Recover:* easy (fallback pins).
*Mitigation:* verify breakout before wiring the amp.

**8. Audio buzz when camera is active.** PCLK coupling into I2S → audible whine.
*Likelihood:* Med. *Detect:* easy (audible). *Recover:* medium (routing/layout).
*Mitigation:* separate I2S from the camera clock; keep wires apart.

**9. Connector / ribbon fatigue on the clip.** The most-handled flex cracks; intermittent
camera or resets. *Likelihood:* Med over time. *Detect:* hard (intermittent). *Recover:*
medium (re-seat/replace). *Mitigation:* strain relief, no cable across the clip joint.

**10. Clip mount stresses the host glasses.** The pod bends or scratches the user's actual
prescription frames. *Likelihood:* Med. *Detect:* easy (visible). *Recover:* none for the
glasses. *Mitigation:* soft clamp, weight limits, per-frame brackets — and a clear warning.

## Network & AI

**11. Wi-Fi won't associate at the venue.** 5 GHz-only AP, captive portal, enterprise
802.1X, or hidden SSID → no data plane. *Likelihood:* High in the wild. *Detect:* easy (no
connect). *Recover:* poor (can't type a portal login on one button). *Mitigation:*
phone-tethering/BLE path; accept "works on known networks only" for V1.

**12. Phone app killed by the OS.** iOS/Android background limits suspend the broker → AI
silently stops. *Likelihood:* High. *Detect:* medium (user just sees "AI unavailable").
*Recover:* easy once noticed (reopen app) — but the magic already broke. *Mitigation:* a
real foreground/companion app (doesn't exist yet); for now, keep the app open.

**13. AI latency 5–8 s.** Every answer feels broken by the third use. *Likelihood:*
Med-High. *Detect:* easy (DEMO_METRICS #4). *Recover:* partial (caching, fewer hops).
*Mitigation:* measure + decompose latency; set expectations to "a few seconds"; this is a
feel-killer more than a function-killer.

**14. API rate limit / quota mid-use.** Provider throttles → errors during a session.
*Likelihood:* Med. *Detect:* medium (intermittent failures). *Recover:* easy (backoff).
*Mitigation:* retry/backoff, surface the state clearly.

**15. API cost surprise.** Multimodal queries add up; the user gets a bill they didn't
expect. *Likelihood:* Med. *Detect:* late (the bill). *Recover:* easy (stop) but
trust-damaging. *Mitigation:* show per-query cost, a usage meter, and a budget cap;
set expectations up front.

**16. API key expired / revoked.** Silent total failure of the value prop. *Likelihood:*
Med over time. *Detect:* poor (looks like "AI unavailable"). *Recover:* easy if diagnosed.
*Mitigation:* distinguish "no key/invalid key" from "no network" in the UI explicitly.

**17. Provider outage.** Claude/OpenAI down → device is just a clock. *Likelihood:* Low per
incident. *Detect:* medium. *Recover:* wait. *Mitigation:* graceful degraded state (already
designed); nothing else possible by definition.

**18. Bad image quality → wrong answers.** Low-res/backlit/motion frames make the AI
confidently wrong. *Likelihood:* Med-High. *Detect:* hard (the answer *looks* fine).
*Recover:* poor — eroded trust doesn't come back. *Mitigation:* image-quality gate
(DEMO_METRICS #9), capture feedback, raise resolution; **the most insidious failure** because
it's invisible and erodes trust silently.

**19. Multimodal refusal.** The model declines on faces, people, documents, or anything it
treats as sensitive → "I can't help with that." *Likelihood:* Med. *Detect:* easy (the
refusal). *Recover:* none for that query. *Mitigation:* set expectations; choose prompts/
provider; can't fully avoid.

**20. Partial frame / SHA mismatch on a flaky link.** Network jitter corrupts the upload →
retries, added latency, occasional failure. *Likelihood:* Med. *Detect:* easy (SHA fails).
*Recover:* easy (retry). *Mitigation:* chunking + SHA already there; bound retries.

## Optics, display & UX

**21. Near-eye image not focusable.** A flat OLED + beam-splitter at 2 cm can't be focused
by the eye → unusable HUD. *Likelihood:* High (the core optics risk). *Detect:* easy (you
can't read it). *Recover:* hard (needs collimation optics). *Mitigation:* buy a known-good
HUD module, or ship audio-only and treat display as research.

**22. HUD invisible in daylight.** Dim monochrome washed out outdoors. *Likelihood:* High
for cheap panels. *Detect:* easy (step outside). *Recover:* medium (brighter source = more
power). *Mitigation:* high-contrast minimal content; accept indoor-first.

**23. One-button confusion → user gets lost.** Can't remember short/long/double; ends up
in the wrong app, can't get back. *Likelihood:* Med-High. *Detect:* hard (user just feels
dumb and quits). *Recover:* easy per-instance (Home), bad for confidence. *Mitigation:*
relentless simplicity, voice, confirmation feedback; cut apps.

**24. Setup never completes.** Entering a Wi-Fi password and an API key on a one-button
device is miserable → user abandons during onboarding. *Likelihood:* High if on-device.
*Detect:* easy (drop-off). *Recover:* n/a (they left). *Mitigation:* phone-assisted setup,
full stop; the on-device wizard is a trap.

**25. Accidental capture / privacy anxiety.** User isn't sure when it's recording; fears
capturing others. *Likelihood:* Med. *Detect:* hard (anxiety, not an error). *Recover:*
medium (clear indicator). *Mitigation:* hardware capture LED, obvious shutter semantics.

**26. Social awkwardness in public.** First odd look at a face-camera-clip → it stays home.
*Likelihood:* High. *Detect:* hard (behavioral). *Recover:* poor. *Mitigation:* discreet
design, assistive framing; accept a narrower audience.

**27. Monochrome / tiny text unreadable.** AI answers truncate or are too small near-eye →
user ignores the display and relies on audio only. *Likelihood:* High on 256×64.
*Detect:* easy. *Recover:* medium (summaries, handoff). *Mitigation:* design for one short
line + "open on phone"; don't promise long text.

**28. Notifications missed or annoying.** Auto-dismiss → missed; or too frequent → noise.
*Likelihood:* Med. *Detect:* medium. *Recover:* easy (settings). *Mitigation:* persistent
dismissible marker, priority floor.

**29. Gesture grammar forgotten after a break.** Returns after days, mis-fires every
press. *Likelihood:* Med. *Detect:* hard. *Recover:* easy (re-learn) but irritating.
*Mitigation:* discoverable affordances, voice as the escape hatch.

## Cost & ownership

**30. "Worse than the phone in my pocket."** Everything Sudonit does, the phone does
faster/clearer; novelty fades. *Likelihood:* High. *Detect:* hard (quiet disuse).
*Recover:* poor. *Mitigation:* find the one thing it's genuinely better at (hands-free,
glanceable) and be ruthless about it.

**31. Charging chore fatigue.** A second tiny thing to charge daily (hourly). *Likelihood:*
High. *Detect:* hard. *Recover:* poor. *Mitigation:* real battery life; until then it's a
desk gadget.

## Maintenance & project

**32. Firmware update bricks the device.** No safe OTA / recovery; a bad flash = dead unit.
*Likelihood:* Med. *Detect:* easy (it's dead). *Recover:* hard for non-makers. *Mitigation:*
A/B OTA + serial recovery before any user has it (not built yet).

**33. Maintainer burnout / repo goes stale.** Solo owner stops; issues pile up; users
stranded. *Likelihood:* Med-High over time. *Detect:* easy (commit gaps). *Recover:* only
via community. *Mitigation:* lower contribution barrier, ship runnable code to attract help.

**34. Dependency rot.** ESP-IDF or provider SDK breaking changes; old builds stop
reproducing. *Likelihood:* Med over a year. *Detect:* medium (build breaks). *Recover:*
medium (pin versions). *Mitigation:* pin toolchain/SDK, CI that actually builds the target.

**35. Docs drift from reality.** The rich architecture diverges from what hardware actually
does; new contributors trust the wrong thing. *Likelihood:* Med-High (already happened once
during the autonomy pivot). *Detect:* hard. *Recover:* medium (reconcile). *Mitigation:*
treat measured reality (DEMO_METRICS) as the source of truth; prune stale design.

---

## The pattern

The deadliest scenarios are not the loud ones (brownout is obvious and fixable). They are
the **silent, trust-eroding, behavioral** ones — **#18 (confidently wrong answers)**,
**#30 (quietly worse than a phone)**, **#26/#31 (it just stays in the drawer)** — because
they have *low detectability* and *poor recoverability*: by the time you notice, the user
is already gone. Every loud hardware failure on this list is more survivable than one
quiet week of the device being not-quite-worth-it.


---

<!-- ===== consolidated from: FIRST_REAL_DATA_PLAN.md ===== -->

# FIRST_REAL_DATA_PLAN.md — the first 10 pieces of evidence to collect when hardware arrives

The moment the board is on the bench, the project stops being a design exercise and starts
being an empirical one. This is the ordered list of the **first 10 pieces of evidence** to
collect — *evidence*, not opinions or impressions.

The distinction is enforced:

> **Evidence is a number, a pass/fail with a defined method, or an artifact (a file, a photo,
> a log) that another person could reproduce. "It feels fast," "looks good," "seems fine" are
> not evidence and are not accepted here.**

Order matters: each datum gates the next, and they are sequenced **cheapest + most-blocking
first**, so a failure stops you before you waste effort downstream. Every item ties to
`ASSUMPTION_REGISTER.md`, `DEMO_METRICS.md`, and `VALIDATION_BACKLOG.md`.

For each: **What evidence · How to measure · What it looks like as data · Records it as · Decision.**

---

### Evidence 1 — The board boots and reports its memory *(A1/A3 · V5)*
- **Measure:** Flash blink + boot-log build; read PSRAM/flash detection from the log.
- **As data:** Boot log text; "PSRAM: 8 MB octal detected: yes/no"; boots 3/3.
- **Records:** `evidence/01-boot.log`.
- **Decision:** If no boot or no PSRAM → stop and fix the board/config before anything else.

### Evidence 2 — Peak current during a full load cycle *(A4 · V6 · DEMO_METRICS #7)*
- **Measure:** Inline USB power meter while firing camera + Wi-Fi + audio together; log resets.
- **As data:** Peak mA, average mA, reset count (target: 0 resets); a current-vs-time trace.
- **Records:** `evidence/02-current.csv` + meter photo.
- **Decision:** This is the **#1 predicted failure.** If it browns out → supply/cap/sequencing work *before* battery design. Feeds Evidence 9.

### Evidence 3 — One intact camera frame *(A2 · V7 · DEMO_METRICS #3)*
- **Measure:** Camera self-test; dump one JPEG over serial/Wi-Fi; open it; SHA-verify.
- **As data:** A viewable `.jpg` file + its SHA; init error code if it fails.
- **Records:** `evidence/03-frame.jpg`.
- **Decision:** If garbage/black → fix pin map/clock. Gates *all* AI evidence below.

### Evidence 4 — Image quality baseline on real scenes *(B9 · DEMO_METRICS #9)*
- **Measure:** Capture 10 fixed scenes (text, object, backlit, motion, low-light); inspect sharpness/exposure.
- **As data:** 10 `.jpg` artifacts + a 1–5 quality score each; count "AI-usable."
- **Records:** `evidence/04-scenes/` + a scoring CSV.
- **Decision:** Sets whether wrong AI answers (FAILURE #18) come from the *image* or the *model*; informs resolution/capture-feedback.

### Evidence 5 — Clean audio out, with the camera active *(A7 · V8 · DEMO_METRICS #5)*
- **Measure:** Play a tone and speech; repeat with the camera streaming; record the output.
- **As data:** An audio capture + pass/fail on "audible buzz with camera active."
- **Records:** `evidence/05-audio.wav`.
- **Decision:** If clean → audio-first V1 is viable (likely path if optics fail). If buzz → routing/layout fix (FAILURE #8).

### Evidence 6 — End-to-end AI round-trip latency *(B10 · V10 · DEMO_METRICS #4)*
- **Measure:** Instrument each hop (capture → upload → model → TTS → playback) over 20 real asks; record per-hop and total.
- **As data:** Median and p90 total ms, plus a per-hop breakdown table.
- **Records:** `evidence/06-latency.csv`.
- **Decision:** **The single most decisive number.** ≤ 2 s median → the flow can win the phone comparison; ~6 s → FIRST_WEEK Day-4. Tells you *which hop* to attack.

### Evidence 7 — AI answer correctness on those real frames *(B9 · V9 · DEMO_METRICS #9)*
- **Measure:** For the same 20 asks, blind-score the answer correct/wrong against ground truth.
- **As data:** Correct fraction (e.g. 13/20) + a list of the wrong ones with the frame attached.
- **Records:** `evidence/07-correctness.csv`.
- **Decision:** Trust verdict. Below ~8/10 and the silent trust-erosion (#18) is real; pair each wrong answer with its frame to see if it's image or model.

### Evidence 8 — Wi-Fi association reliability, including one hostile network *(B11 · V12 · DEMO_METRICS #2)*
- **Measure:** 10 connect attempts on home Wi-Fi + 1 attempt each at a phone hotspot and a captive-portal/5 GHz venue.
- **As data:** Associate time (s) + success count per network type.
- **Records:** `evidence/08-wifi.csv`.
- **Decision:** Confirms/denies "works on known networks only" (FAILURE #11); decides if BLE/tether becomes mandatory.

### Evidence 9 — Real battery rundown on a wearable cell *(A5 · V11 · DEMO_METRICS #6/#8)*
- **Measure:** With Evidence-2 currents and a chosen LiPo, run continuous-use and mixed-use rundowns to cutoff.
- **As data:** Minutes of continuous + mixed-use runtime; average mA confirmed.
- **Records:** `evidence/09-battery.csv`.
- **Decision:** **Wearable vs. desk-bound.** < 1 h continuous reframes V1 honestly and prioritizes duty-cycle/BLE (FAILURE #2/#31).

### Evidence 10 — Thermal at the temple after sustained use *(A6 · V13 · DEMO_METRICS reference)*
- **Measure:** IR thermometer / thermocouple on the enclosure after 15–30 min sustained use; note ambient.
- **As data:** Enclosure °C above ambient + a subjective "wearable yes/no" wear note.
- **Records:** `evidence/10-thermal.csv` + IR photo.
- **Decision:** Wearability ceiling; whether the load cycle must be throttled before any user wears it.

---

## How this data is handled (the discipline that makes it evidence-driven)

- **Store every artifact** under `evidence/` in the repo, named by item, with the raw file
  (log, csv, jpg, wav, photo). An artifact someone else can re-open is the difference between
  evidence and a claim.
- **Update `TRUTH_TABLE.md`** the same day: each datum moves its belief out of **Unknown** to
  Known True / Known False — *with the artifact linked.* Nothing moves columns without evidence.
- **Update `ASSUMPTION_REGISTER.md` confidences** to reflect the measurement, not the hope.
- **`DEMO_METRICS.md` is the source of truth** where it and a doc disagree — measured reality
  wins, design prose loses (FAILURE #35: docs drift from reality).

## What these 10 deliberately do NOT include yet

Optics (C14) and the 7-day Sean-Ellis verdict (D20) are *not* in the first 10 — not because
they're unimportant (they're the most important) but because they're gated: optics is a
*procurement + bench* experiment running in parallel (V4), and the wear-test is meaningless
until Evidence 1–10 prove the device is worth a week of someone's face. The first 10 exist to
earn the right to run V16.

## The point of all ten

> After these ten measurements, the project's central column flips. Most of the beliefs that
> decide whether Sudonit is a product — brownout, latency, correctness, battery, thermal,
> field Wi-Fi — stop being **Unknown** and become **facts with files attached.** From that
> moment the project is evidence-driven, and every further decision can cite a number instead
> of a hope. That transition, not any single feature, is the real milestone hardware unlocks.


---

<!-- ===== consolidated from: DEMO_METRICS.md ===== -->

# DEMO_METRICS.md

The exact measurements to collect the day hardware arrives. The purpose is narrow and
deliberate: **replace opinions with numbers.** Every claim in the audit (`PROJECT_AUDIT.md`,
`TOP_RISKS.md`) about latency, power, and image quality is currently a guess; this document
is the instrument list that turns each guess into a recorded figure.

This adds **no features and no architecture** — it only defines what to measure, against
what thresholds, with what method. Run it alongside `DEMO_DAY_PLAN.md` (same step order).

## How to use this

- Fill the **Result** column on the bench. One number per metric, plus a pass/fail.
- Thresholds: **Target** = the product would feel good · **Acceptable** = usable, note it ·
  **Failure** = stop and fix before moving on; this is a real problem, not a tuning detail.
- Prefer the **simplest honest method**: a phone stopwatch and a USB power meter beat
  nothing. Note the method actually used next to each result.
- If a metric can't be measured yet (e.g. no battery cell present), write **N/A — reason**.
  A documented gap is a result; a blank is not.

### Bench instruments needed
USB-C power meter (inline, mA + mWh), a phone/stopwatch, the host machine for timestamp
logging, the seller's camera pin diagram, a known 2.4 GHz AP, `ANTHROPIC_API_KEY` exported,
and (optionally) a LiPo + charger. Where possible, log timestamps in firmware/server over
serial so timing isn't eyeballed.

---

## 1. Boot time
- **What:** power-on / reset → app_main running and the boot/HAL log printed (default build).
- **Target:** ≤ 2 s · **Acceptable:** ≤ 5 s · **Failure:** > 10 s or boot loop.
- **Method:** timestamp from reset edge to first app log line over serial monitor; average
  of 3 cold boots. (For a wearable, also note wake-from-idle later, but day one is cold boot.)

## 2. Wi-Fi connect time
- **What:** `sd_net_start` called → STA associated + IP acquired (ready to TCP-connect).
- **Target:** ≤ 3 s · **Acceptable:** ≤ 8 s · **Failure:** > 15 s or intermittent (< 90%
  of attempts associate).
- **Method:** firmware timestamps association-start → `GOT_IP`; run 10 attempts, record
  mean and success rate. Use a controlled 2.4 GHz AP. **Never log the password.**

## 3. Capture latency (device-local)
- **What:** capture trigger → JPEG frame available in PSRAM (camera only, no network).
- **Target:** ≤ 200 ms · **Acceptable:** ≤ 500 ms · **Failure:** > 1 s, or frame size
  implausibly small / decode fails.
- **Method:** firmware timestamps `esp_camera_fb_get()` call → return; record frame byte
  size too. 10 captures; report mean + min/max. Note resolution (SVGA unless changed).

## 4. AI round-trip latency (the headline number)
- **What:** capture trigger → AI answer text received back on the device (full loop:
  capture → ESP32 → Wi-Fi → phone → Claude → answer → device). Audio excluded (metric 5).
- **Target:** ≤ 2 s · **Acceptable:** ≤ 4 s · **Failure:** > 6 s, or > 1-in-10 requests
  error/timeout.
- **Method:** single clock — log `t0` at trigger on the device, `t1` at `ai_response`
  receipt; the phone server logs its own send/receive + the provider call duration so the
  total can be decomposed into **on-device + network-to-phone + provider** (knowing *where*
  the seconds go matters as much as the total). 20 queries over a normal connection; report
  median, p90, and the decomposition. Also record **provider cost per query** if available.

## 5. Audio latency
- **What:** AI answer text ready on device → first audible sample from the speaker (TTS on
  phone → PCM downlink → `sd_audio_play_pcm`).
- **Target:** ≤ 500 ms · **Acceptable:** ≤ 1 s · **Failure:** > 2 s, audible gaps/
  underruns, or buzz when the camera is active.
- **Method:** timestamp answer-ready → first I2S write; confirm audibly. Separately note
  any **camera-active buzz** (PCLK coupling) as pass/fail. 10 playbacks.

## 6. Idle current
- **What:** average supply current with the device booted, Wi-Fi associated, display (if
  any) on its idle screen, no capture/audio.
- **Target:** ≤ 80 mA · **Acceptable:** ≤ 150 mA · **Failure:** > 250 mA, or unstable.
- **Method:** inline USB power meter, 60 s average after settling. Record with Wi-Fi on and
  (separately) Wi-Fi off to quantify the radio's idle cost — this directly informs the
  "Wi-Fi is the wrong transport" risk (TOP_RISKS #13).

## 7. Active current (peak + sustained)
- **What:** supply current during a full capture → uplink → audio cycle; capture both the
  **peak** (the brownout risk) and the **sustained** draw during Wi-Fi TX + audio.
- **Target:** peak ≤ 500 mA, sustained ≤ 300 mA · **Acceptable:** peak ≤ 800 mA ·
  **Failure:** peak causes a **brownout/reset**, or sustained > 1 A.
- **Method:** USB meter with peak-hold (or a scope on a shunt if available) across 10 full
  cycles; note any reset. **This is the single most important power number** — it decides
  whether the device survives camera+Wi-Fi+audio together (TOP_RISKS #9).

## 8. Battery life estimate
- **What:** projected runtime from a representative usage mix, derived from metrics 6–7.
- **Target:** ≥ 4 h mixed use · **Acceptable:** ≥ 1 h · **Failure:** < 30 min, or thermal
  (enclosure/temple uncomfortably warm).
- **Method:** if a cell is present, measure mWh draw over a 10-min scripted mix (e.g. 1
  query/min + idle) and extrapolate to the cell's capacity; also do a real drain-to-cutoff
  if time allows. If no cell: compute the estimate from metrics 6–7 and a candidate mAh,
  and mark **estimate (no cell tested)**. Record surface temperature after 10 min of active
  use regardless.

## 9. Image quality checks
- **What:** is the captured frame good enough for reliable AI answers? Qualitative but
  recorded against fixed checks, not vibes.
- **Target:** ≥ 8/10 checks pass · **Acceptable:** ≥ 6/10 · **Failure:** ≤ 4/10, or the AI
  visibly misreads typical scenes.
- **Method:** capture a fixed test set (a printed paragraph at ~1 m, a street-sign-like
  high-contrast text, a colored object, an indoor scene, a backlit scene) and score each:
  | Check | Pass if |
  |---|---|
  | Exposure (indoor) | subject neither blown out nor crushed |
  | Exposure (backlit) | subject still legible |
  | Focus / sharpness | printed text edges crisp at ~1 m |
  | Readable text | a short printed paragraph is human-readable in the JPEG |
  | Color fidelity | a known-colored object is recognizably that color |
  | Motion (slight) | a small head-turn doesn't smear the frame unusably |
  | Field of view | the intended subject is actually framed |
  | JPEG integrity | decodes cleanly, no corruption/artifacts |
  | File size sane | not implausibly tiny/huge for the scene |
  | **AI agreement** | Claude's answer about the frame is correct for ≥ 4/5 test scenes |
  The last row is the one that matters most: it tests the *whole* value proposition on a
  real frame (TOP_RISKS #7, #21).

---

## Results table (fill on the bench)

| # | Metric | Target | Acceptable | Failure | Result | Pass? | Method/notes |
|---|---|---|---|---|---|---|---|
| 1 | Boot time | ≤2 s | ≤5 s | >10 s | | | |
| 2 | Wi-Fi connect | ≤3 s | ≤8 s | >15 s / <90% | | | |
| 3 | Capture latency | ≤200 ms | ≤500 ms | >1 s | | | |
| 4 | AI round-trip | ≤2 s | ≤4 s | >6 s / >10% err | | | median + p90 + split |
| 5 | Audio latency | ≤500 ms | ≤1 s | >2 s / buzz | | | |
| 6 | Idle current | ≤80 mA | ≤150 mA | >250 mA | | | Wi-Fi on/off |
| 7 | Active current | peak ≤500 mA | peak ≤800 mA | brownout | | | **peak-hold** |
| 8 | Battery life | ≥4 h | ≥1 h | <30 min | | | + temp |
| 9 | Image quality | ≥8/10 | ≥6/10 | ≤4/10 | | | scene scores |

---

## The four numbers that decide everything

If the day produces nothing else, get these — they are the figures the audit says do not
exist and that separate "product" from "project":

1. **Active peak current** (#7) — does it brown out? (reliability)
2. **AI round-trip median + p90** (#4) — does the magic feel instant or broken? (UX)
3. **AI-agreement on real frames** (#9, last row) — is the answer actually right? (value)
4. **Battery life / draw** (#8) — can it be worn off a wall socket? (wearability)

Record those four before touching the display, the UI runtime, or anything cosmetic.


---

<!-- ===== consolidated from: DEMO_SUCCESS_CRITERIA.md ===== -->

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
