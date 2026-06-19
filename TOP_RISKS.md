# TOP_RISKS.md

The 25 highest project risks, ranked highest → lowest by combined severity × likelihood
× impact. Each entry: **severity** (how bad if it lands), **likelihood** (will it land),
**impact** (what it hits), **mitigation** (the honest, often unsatisfying, option), and
**when it becomes critical** (the moment it can no longer be deferred).

Severity/likelihood scale: Low / Medium / High / Critical.

This supersedes the older top-level `RISKS.md` as the current, prioritized list.

---

### 1. No market / no business model (BYO-API-key niche)
- **Severity:** Critical · **Likelihood:** High · **Impact:** The whole project's reason to exist
- **Why:** Requiring each user to bring and pay for an AI key caps the audience at developers; there is no recurring revenue, no mass market, no funding case.
- **Mitigation:** Reframe explicitly as an open dev platform / learning project, *or* find a managed-AI + business model — but that contradicts the privacy premise. There is no comfortable fix.
- **Critical:** Now. Every downstream effort compounds this if unaddressed.

### 2. Nothing has been validated on hardware
- **Severity:** Critical · **Likelihood:** High (something will fail) · **Impact:** Entire technical premise
- **Why:** All drivers are stubs/untested; the AI loop has never run with a real model. "Readiness %" measures paper, not silicon.
- **Mitigation:** Stop designing; execute DEMO_DAY_PLAN the moment hardware lands; treat first-light as the real project start.
- **Critical:** The day hardware arrives.

### 3. Power / battery / thermal for a wearable
- **Severity:** Critical · **Likelihood:** High · **Impact:** Whether it's wearable at all
- **Why:** No power budget exists. Camera+Wi-Fi+audio+display on a behind-ear LiPo likely yields tens of minutes and noticeable heat.
- **Mitigation:** Compute a real mAh budget before V2; measure current on day one; consider duty-cycling and BLE.
- **Critical:** Before any untethered demo or V2 enclosure.

### 4. Near-eye optics may be physically unbuildable (solo)
- **Severity:** Critical · **Likelihood:** Medium-High · **Impact:** The entire display/UX investment
- **Why:** A cheap flat OLED + beam-splitter isn't focusable without collimation; aligning an eyebox on a clip-on per face is lab-grade work.
- **Mitigation:** Buy a known-good HUD module instead of building optics; or ship audio-only V1 and treat display as research.
- **Critical:** Before trusting any of the UI-runtime/app work as product-bound.

### 5. Loop latency + multi-hop fragility
- **Severity:** High · **Likelihood:** High · **Impact:** The core "magic" feel
- **Why:** 5-hop chain, 2–6 s, breaks when phone sleeps / Wi-Fi drops / no internet.
- **Mitigation:** Measure real latency; cache; make degraded state unmissable; lower expectations from "instant" to "a few seconds."
- **Critical:** First real Claude run.

### 6. Scope creep + solo bus factor (project velocity death)
- **Severity:** High · **Likelihood:** High · **Impact:** Whether the project ever finishes
- **Why:** Autonomy-first added a whole UI runtime + 8 apps as unbuilt scope; one person owns everything; docs already drifted out of sync once.
- **Mitigation:** Freeze new design; cut apps to the one core flow; write running code; recruit contributors.
- **Critical:** Now — every sprint that adds docs instead of validated code deepens it.

### 7. Real Claude / image quality unvalidated
- **Severity:** High · **Likelihood:** Medium-High · **Impact:** The value proposition
- **Why:** SVGA JPEG from OV5640 + cheap optics may be too poor for reliable VQA; cost/latency/refusals unknown.
- **Mitigation:** Run real Claude with sample frames *today* (host) before hardware; measure answer quality and cost.
- **Critical:** Immediately — it needs only an API key.

### 8. Social rejection of a conspicuous face camera
- **Severity:** High · **Likelihood:** Medium-High · **Impact:** Adoption
- **Why:** A clip-on camera is more obvious/awkward than an integrated frame; the Glass stigma applies harder.
- **Mitigation:** Visible capture indicator; lean into assistive/accessibility framing; accept it limits the audience.
- **Critical:** First public wearing / any launch.

### 9. Brownout on first power-up
- **Severity:** High · **Likelihood:** High · **Impact:** Day-one bring-up
- **Why:** Camera + Wi-Fi current spikes reset under-provisioned supplies.
- **Mitigation:** USB power, bulk capacitance, a quality supply, USB current meter; bring up subsystems one at a time.
- **Critical:** Day one, step "camera + Wi-Fi together."

### 10. Clip-on form-factor physics (balance/fit/alignment)
- **Severity:** High · **Likelihood:** Medium-High · **Impact:** Whether it stays on a face usefully
- **Why:** Mass behind the ear vs. camera/optics forward, on arbitrary glasses, is an unbounded mechanical problem.
- **Mitigation:** Prototype on one reference pair; accept per-model brackets; reconsider a dedicated frame.
- **Critical:** First physical pod build.

### 11. ESP32-S3 resource exhaustion (everything at once)
- **Severity:** High · **Likelihood:** Medium · **Impact:** Whether the full config runs
- **Why:** Camera FB + Wi-Fi + I2S + UI runtime + fonts + heap buffers concurrently, unmeasured.
- **Mitigation:** Measure RAM/PSRAM with all flags on; cut the UI runtime scope; lower camera resolution.
- **Critical:** When camera + audio + net + (future) display are first enabled together.

### 12. Certification & legal (FCC/CE/UN38.3/recording law)
- **Severity:** High · **Likelihood:** Medium (if it ever sells) · **Impact:** Legality of distribution
- **Why:** Wireless camera wearable + LiPo + recording = regulated on multiple axes; entirely unaddressed.
- **Mitigation:** Keep it a personal/open project until/unless funded; budget certification before any sale.
- **Critical:** The moment money changes hands.

### 13. Wi-Fi is the wrong transport; BLE unbuilt (debt + power)
- **Severity:** Medium-High · **Likelihood:** High · **Impact:** Power, pairing UX, product-ness
- **Why:** Wi-Fi STA is power-hungry and network-dependent; BLE control plane is 0% built but assumed.
- **Mitigation:** Budget BLE work; accept Wi-Fi as a bench transport only.
- **Critical:** First untethered / "pairs like an accessory" claim.

### 14. Consumer phone app does not exist
- **Severity:** Medium-High · **Likelihood:** High · **Impact:** Usability for non-developers
- **Why:** A Python server ≠ a maintained iOS+Android app with BLE + background limits + store review.
- **Mitigation:** Scope the app as its own project; until then, audience = CLI-comfortable users.
- **Critical:** Any non-developer user.

### 15. Camera pin map assumption wrong
- **Severity:** Medium · **Likelihood:** Medium · **Impact:** Day-one camera bring-up
- **Why:** Pins are a "probably Freenove" guess on an AliExpress board.
- **Mitigation:** Verify against seller diagram before first flash; the driver supports re-mapping.
- **Critical:** Day one, camera step.

### 16. Simulator validates a fiction (color/emoji/text)
- **Severity:** Medium · **Likelihood:** High · **Impact:** Confidence in UX decisions
- **Why:** The "primary validation tool" renders what a 1-bit panel can't.
- **Mitigation:** Add a V1-honest mode (1-bit, no emoji, capped text) before trusting UX conclusions.
- **Critical:** Before any UX decision is treated as settled.

### 17. PSRAM / sdkconfig mismatch on silicon
- **Severity:** Medium · **Likelihood:** Low-Medium · **Impact:** Camera + memory
- **Why:** Octal PSRAM assumed from the label, never confirmed on this unit.
- **Mitigation:** Confirm in boot log; sdkconfig already targets N16R8.
- **Critical:** Day one, boot step.

### 18. One-button UX doesn't scale
- **Severity:** Medium · **Likelihood:** Medium · **Impact:** Everyday usability
- **Why:** O(n) launcher; secondary tasks 4–6 actions; overloaded PRIMARY.
- **Mitigation:** Add input modes (voice/encoder); recents/favorites; cut apps.
- **Critical:** As the app set grows / at real daily use.

### 19. I2S audio pins not broken out / camera-clock noise
- **Severity:** Medium · **Likelihood:** Low-Medium · **Impact:** Audio output
- **Why:** GPIO 40/41/42 may not be exposed; PCLK coupling can buzz.
- **Mitigation:** Verify breakout; fallback pins; route away from PCLK.
- **Critical:** Day one, audio step.

### 20. Display hardware not even ordered
- **Severity:** Medium · **Likelihood:** High (it's true now) · **Impact:** Schedule realism
- **Why:** Months of UX/runtime work precede buying the part it targets.
- **Mitigation:** Order a candidate panel now or formally defer display to a later phase.
- **Critical:** Whenever "display demo" is promised.

### 21. OV5640 SVGA image quality insufficient for VQA
- **Severity:** Medium · **Likelihood:** Medium · **Impact:** AI answer quality
- **Why:** Low resolution + cheap optics + motion + lighting may degrade model accuracy.
- **Mitigation:** Test real frames against the model; raise resolution within memory limits.
- **Critical:** First real Claude run.

### 22. Open-hardware abandonment / no community
- **Severity:** Medium · **Likelihood:** Medium-High · **Impact:** Longevity
- **Why:** Most open-hardware projects stall; there is no community yet.
- **Mitigation:** Lower the contribution barrier; ship something runnable to attract help.
- **Critical:** When solo bandwidth runs out.

### 23. Privacy law for bystanders (beyond the user)
- **Severity:** Medium · **Likelihood:** Low-Medium · **Impact:** Legal/social
- **Why:** "Your key protects *you*"; it does nothing for recorded bystanders / consent laws.
- **Mitigation:** Capture indicator; documentation; jurisdiction awareness.
- **Critical:** Public use / distribution.

### 24. BOM not actually cheap at volume
- **Severity:** Low-Medium · **Likelihood:** Medium · **Impact:** The "cheap/open" claim
- **Why:** qty-1 AliExpress prices ≠ a manufactured, cased, certified unit.
- **Mitigation:** Build a real costed BOM including enclosure/cert/assembly before claiming a price.
- **Critical:** Any pricing/marketing claim.

### 25. Protocol hand-maintained across C/Python
- **Severity:** Low · **Likelihood:** Medium · **Impact:** Maintenance tax
- **Why:** Byte-compatibility is verified by hand on every change.
- **Mitigation:** Keep the interop tests; consider codegen if it grows.
- **Critical:** As the protocol expands.

---

**Reading the ranking:** risks 1–6 are existential or velocity-killing and are mostly
*not* engineering problems — they are about market, validation, power, optics, and
focus. Risks 9, 15, 17, 19 are concrete day-one bring-up hazards with clear fallbacks.
The pattern is the same as the audit's: the dangerous risks are the ones the project has
been *least* inclined to work on, because they are the least pleasant to design around.
