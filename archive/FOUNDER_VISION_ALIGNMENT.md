# FOUNDER_VISION_ALIGNMENT.md — repo vs. canonical founder vision

Purpose: not new architecture, not new features. A sweep that compares the **canonical
founder vision** against what the repository actually assumes today, and flags where they
diverge. The goal is that the repo reflects the *intended* product, not assumptions that
hardened during architecture work.

**Headline:** the repo is already ~80% aligned. The big pillars — autonomy-first, a *module*
that clips onto the user's own glasses, modularity/repairability as features, phone-as-extension,
provider-agnostic AI — are already canon in `VISION.md`, `PRODUCT_VISION.md`,
`FRAME_ARCHITECTURE.md`, `MVP_DEFINITION.md`. The divergences below are narrow but consequential,
and one of them (the AI-access model) materially changes how grim the project's own audit reads.

---

## 1. Which existing repository assumptions CONFLICT with the vision

### 1.1 AI access is encoded as "Bring-Your-Own-Key, period" — the vision says it's one of several options
**This is the most important mismatch.** Founder point 5/6: *privacy-friendly, not cloud-free*;
the user **may** use Sudonit-hosted AI, **or** their own key, **or** their own/self-hosted server.
Hosted AI is explicitly an *optional paid service* (a revenue line), and no specific provider is forced.

The repo only encodes BYO-key:
- `PRODUCT_VISION.md` (V2): "the user's own API key — privacy by construction." No Sudonit-hosted path mentioned.
- `CLOUD.md` lists third-party providers (OpenAI/Anthropic/Gemini/local-research) but **no Sudonit-hosted
  option** and no "user's own server" path.
- `ASSUMPTION_REGISTER.md` **D19** treats BYO-key as *the* model, scores it 20%, and calls it the
  "killer question … an audience, not a market." `WHY_SUDONIT_LOSES #1` and the `PROJECT_AUDIT`
  killer-question inherit that framing.

**Why it matters:** the project's darkest self-assessment assumes *forced* BYO-key — that normal users
must obtain and manage an API key and a bill. The vision removes that premise: normal users get a
**Sudonit-hosted default**; makers/privacy users bring a key or point at their own server. D19's doom is
largely an artifact of an assumption the founder does not hold. The "market vs. audience" verdict should
be re-scored against *hosted-default*, not BYO-only.

### 1.2 An on-device **visual** UI runtime is pulled into V1; the vision puts the display in V2
Founder hardware roadmap: **V1 = ESP32-S3 + camera + audio + AI**. *Display* is a **V2** item
("better display"). But the repo elevates a pixel-drawing runtime to a V1 **must-have**:
- `MVP_DEFINITION.md` must-have **A1**: "On-device UI runtime — local rendering … the glasses draw their
  own pixels … *(New firmware — embraced, not minimized.)*" and calls it "the new work this philosophy adds."
- `DISPLAY_ARCHITECTURE.md §0` is cited as the **governing principle** for autonomy itself, fusing the
  autonomy argument to a rendering runtime.

The lowest-confidence assumptions in the whole project sit exactly here: **C14 focusable near-eye optics
(15%)**, **C16 readable mono text (35%)**, plus the UI-runtime firmware. Under the vision, V1 autonomy is
expressed through **audio + one button + local state** (clock, settings, capture, storage) — *heard and
held*, not *rendered*. A pixel UI runtime is V2-aligned work.

*Precision:* `MVP_DEFINITION.md` already permits a "headless (audio + capture)" fallback, so this is a
conflict of **requirement level and emphasis**, not a flat contradiction. The fix is to demote the visual
runtime from "must-have / the philosophy's headline new work" to "V2," and let V1 autonomy be audio-first.

### 1.3 "Compete with / lose to Ray-Ban Meta" framing measures the wrong axis
Founder point 1: Sudonit is **NOT** trying to be Ray-Ban Meta; the analogy is **ThinkPad** —
repairable, modifiable, owner-controlled. Yet:
- `VISION.md` Long-Term Goal: "compete with Ray-Ban Meta."
- `WHY_SUDONIT_LOSES.md` / `docs/COMPETITOR_ANALYSIS.md` grade Sudonit largely on Ray-Ban's **polish**
  axis (finish, ecosystem, social acceptability).

`PRODUCT_VISION.md` already self-corrects ("contests Ray-Ban Meta on *openness and repairability*, not
pixel count"), so it's the **older** framing (the VISION long-term line + the loss analysis) that is now
partially outdated. Grading a deliberately open, repairable module against a closed consumer gadget's
polish is scoring the wrong game.

### 1.4 Module portability is logged as a liability, not the differentiator it is
Founder point 2: keeping your own glasses and **moving the module between frames** is a *core
differentiator*. The repo logs the same fact defensively: `ASSUMPTION_REGISTER` **A8** ("fits every
frame," 30%) and `WHY_SUDONIT_LOSES` treat clip ergonomics as an unbounded mechanical headache. Both are
true — but the *strategic value* of solving it is higher than the repo credits. The risk is unchanged; the
payoff is under-weighted.

---

## 2. Which documents are now PARTIALLY OUTDATED

| Document | What's stale | Direction of fix |
|---|---|---|
| `CLOUD.md` | No Sudonit-hosted AI option; no "your own server / self-hosted" path. | Add hosted + self-hosted as first-class peers to BYO-key. |
| `PRODUCT_VISION.md` (V2 AI line) | "your own API key" presented as *the* model. | Present hosted-default + BYO + self-host as choices. |
| `VISION.md` | "compete with Ray-Ban Meta"; V1 Goals omit the AI-access choice and the hosting option. | Reframe to ThinkPad/owner-controlled axis; note hosted/BYO/self-host. |
| `ASSUMPTION_REGISTER.md` D19 (+ `WHY_SUDONIT_LOSES` #1, `PROJECT_AUDIT`) | Assume *forced* BYO-key → "audience not a market." | Re-score against a hosted default; BYO becomes the privacy/maker path, not the only path. |
| `MVP_DEFINITION.md` A1 + `DISPLAY_ARCHITECTURE.md §0` | Visual UI runtime as a V1 must-have and the autonomy headline. | Demote rendering to V2; V1 autonomy = audio + button + local state. |
| `docs/DISPLAY_*`, `docs/UI_RUNTIME_ARCHITECTURE.md`, `docs/UI_STATE_MACHINE.md` | V2 artifacts currently treated as V1 scope. | Re-label as V2; freeze, don't extend, for V1. |

(No edits made here — this report only flags. Per the founder's instruction, no new design docs and no
rewrites until you decide.)

---

## 3. Which future DESIGN DIRECTIONS should change

1. **Make AI access a 3-way choice, hosted-first for normals.** First-class: (a) Sudonit-hosted AI
   (optional paid), (b) user's own API key, (c) user's own/self-hosted server. The provider layer already
   abstracts this (`phone/ai/provider.py` `get_provider()`); the *product* framing and `CLOUD.md` need to
   catch up to the code's flexibility.
2. **V1 is audio-first; the visual UI runtime is V2.** Stop spending the scarce pre-hardware budget on the
   15%-confidence optics/HUD/UI-runtime track. Autonomy in V1 = the device knows the time, holds settings,
   captures, stores, and answers by voice — phone off.
3. **Re-axis the competitive story** from "a cheaper Ray-Ban" to "the owner-controlled, repairable,
   portable module." Judge V1 against *ownership/modularity/repairability*, not polish.
4. **No mandatory subscription, ever, in any doc.** Hardware keeps working after purchase; paid services
   are convenience, not a gate. This belongs as an explicit product constraint (currently unstated anywhere).
5. **Treat ESP32-S3 as a stepping stone, not the platform.** V3 may be Qualcomm/Linux-class. Keep the HAL
   boundary clean; don't optimize firmware *for* the S3 in ways that would have to be unwound.

---

## 4. Which decisions become STRONGER because of this vision

- **The HAL / swappable-backend architecture** (interfaces + esp32/host/mock backends) is *exactly* the
  right call for "modularity is a product feature" and for the V1→V3 SoC change. Vindicated.
- **Autonomy-first** (`VISION.md` Core Principle 1, memory `device-autonomy-first`) — reaffirmed verbatim
  by founder point 4. No change needed.
- **Provider-agnostic AI** — `CLOUD.md`'s "cloud providers are replaceable; never depend on a single
  vendor" directly serves "never force a specific provider." Strengthened (and should grow a hosted peer).
- **Repairability built into the physical design** — `FRAME_ARCHITECTURE.md §6` (connectors over solder,
  slide-out battery, separable camera/compute/optic modules, replaceable clip) is the ThinkPad philosophy
  in hardware. Keep it; it's a selling point, not overhead.
- **Audio-only V1 as a valid product** — previously framed as a fallback; the vision promotes it to the
  *primary* V1 path. The work already done on the audio loop is now core, not contingency.
- **Phone-brokered AI** — the "cloud AI is allowed because inference genuinely exceeds the device" stance
  matches `VISION.md` non-goals exactly. Unchanged and correct.

---

## Bottom line

The repository's *bones* already match the founder's product. Two corrections actually move the project:
(1) **AI access is hosted-or-BYO-or-self-host, not BYO-only** — which defuses the audit's single most
pessimistic conclusion; and (2) **V1 is audio-first, the visual display/UI-runtime is V2** — which frees
the pre-hardware budget away from the 15%-confidence optics gamble toward the loop that actually defines
V1. Everything else is relabeling V2 work as V2 and re-axing the competitive story onto ownership.
