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
