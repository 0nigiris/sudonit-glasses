# DOCS_CONSOLIDATION_PLAN.md

> Cleanup plan to cut documentation sprawl **without losing information**. The repo
> currently holds **71 project Markdown files** (excluding dependencies and
> `.pytest_cache`) — more docs than is navigable, with heavy duplication across risks,
> demo/launch, vision, and pre-hardware backlogs.
>
> This is a **repository cleanup plan**, not new product/vision/architecture/audit
> content. Every conclusion and decision is preserved: historical files move to
> `archive/` intact; overlapping files merge into a canonical target; only pure
> duplicates are deleted (their content already living verbatim in a canonical doc).
>
> **Execution status:** the lossless half is done in this change — the 21 historical
> files below are moved to `archive/`. The content **merges** (32 files) and **deletes**
> (4 files) are specified here but intentionally *not* executed, because merging prose
> risks losing nuance and deserves review. They are the approved-next-step.

---

## Target final structure

The 7 documents a new contributor reads to understand the whole project:

```
README.md            entry point — what it is, how to run, where to go
VISION.md            why Sudonit exists, founder vision, principles
ARCHITECTURE.md      system design (glasses + phone, HAL, services, app model)
MVP.md               what V1 is and is not
DEVELOPMENT.md       build, run, test, contribute, pre-hardware task plan
VALIDATION.md        assumptions, risks, failure modes, evidence to collect
DEMO_AND_LAUNCH.md   demo script/video/photos, launch readiness, first posts
```

Supporting active docs (read as needed, not required for the overview):

```
PRODUCT.md           product briefing (from PROJECT_BRIEF_SUDONIT.md)
WEBSITE.md           canonical website content (from WEBSITE_CONTENT_MASTER.md)
PROTOCOL.md          wire-protocol specification
DECISIONS.md         decision log (ADR-style) — preserves every decision
ROADMAP.md           V1/V2/V3 roadmap + milestones
HARDWARE.md          board resources, bring-up, provisioning, references
```

**= 13 active documentation files** (within the 10–15 target).

Infrastructure / component-scoped (not "documentation sprawl", stay in place):

```
CLAUDE.md                    agent operating instructions
CONTRIBUTING.md              trimmed to a pointer into DEVELOPMENT.md (GitHub convention)
firmware/README.md           firmware build (scoped)
firmware/esp-idf/README.md   ESP-IDF target build (scoped)
protocol/TRANSPORT.md        data-plane spec (scoped beside the code)
archive/                     historical reference (intact, never deleted)
```

---

## Duplication detected (the reason this is needed)

| Cluster | Files today | Consolidated into |
|---------|-------------|-------------------|
| **Risk / assumptions** | `RISKS.md`, `TOP_RISKS.md`, `ASSUMPTION_REGISTER.md`, `FAILURE_SCENARIOS.md`, `TRUTH_TABLE.md` | `VALIDATION.md` |
| **Validation / evidence** | `VALIDATION_BACKLOG.md`, `FIRST_REAL_DATA_PLAN.md`, `DEMO_METRICS.md`, `DEMO_SUCCESS_CRITERIA.md` | `VALIDATION.md` |
| **Demo / launch** | `DEMO_SCRIPT.md`, `DEMO_VIDEO_PLAN.md`, `DEMO_DAY_PLAN.md`, `PHOTO_SHOTLIST.md`, `LAUNCH_READINESS.md`, `FIRST_PUBLIC_POSTS.md` | `DEMO_AND_LAUNCH.md` |
| **Vision (repeated)** | `VISION.md`, `docs/PRODUCT_VISION.md`, `MASTER.md`, `FOUNDER_VISION_ALIGNMENT.md` | `VISION.md` (+ analysis archived) |
| **Pre-hardware backlog** | `QUICK_WINS.md`, `CODE_FIRST_BACKLOG.md`, `PRE_HARDWARE_EXECUTION_PLAN.md`, `PRE-HARDWARE_STOP_LIST.md` | `DEVELOPMENT.md` |
| **Hardware bring-up** | `HARDWARE_ARRIVAL_CHECKLIST.md`, `docs/HARDWARE_INTEGRATION_PLAN.md`, `docs/CAMERA_BRINGUP.md`, `docs/AUDIO_BRINGUP.md`, `docs/PROVISIONING_PLAN.md`, `docs/BOARD_RESOURCES.md`, `REFERENCES.md` | `HARDWARE.md` |
| **Roadmap** | `ROADMAP.md`, `MILESTONES.md`, `docs/PRE_HARDWARE_ROADMAP.md`, `FUTURE_ROADMAP_REVIEW.md` | `ROADMAP.md` (+ review archived) |
| **Audits / reviews (historical)** | `PROJECT_AUDIT.md`, `PRODUCT_REVIEW.md`, `WEBSITE_REVIEW.md`, `IMPLEMENTATION_GAP_REPORT.md` | archive / `VALIDATION.md` |
| **Display (V2 — not in V1)** | `docs/DISPLAY_ARCHITECTURE.md`, `docs/DISPLAY_BRINGUP_PLAN.md`, `docs/DISPLAY_EVALUATION.md`, `docs/UI_RUNTIME_ARCHITECTURE.md`, `docs/UI_STATE_MACHINE.md` | `archive/` |

---

## Per-file disposition

Categories: **Canonical** (stays active), **Merge** (folds into a target, husk removed),
**Archive** (moved to `archive/` intact), **Delete** (pure duplicate, content already in
a canonical doc).

### Canonical — stays active (14)

| Current path | Category | Reason | Final name |
|---|---|---|---|
| `README.md` | Canonical | Entry point | `README.md` (rewrite per archived plan) |
| `VISION.md` | Canonical | Canonical vision | `VISION.md` |
| `ARCHITECTURE.md` | Canonical | Canonical system design | `ARCHITECTURE.md` |
| `PROTOCOL.md` | Canonical | Wire-protocol spec | `PROTOCOL.md` |
| `DECISIONS.md` | Canonical | Decision log — preserves decisions | `DECISIONS.md` |
| `ROADMAP.md` | Canonical | Roadmap (absorbs milestones) | `ROADMAP.md` |
| `PROJECT_BRIEF_SUDONIT.md` | Canonical | Product briefing | → `PRODUCT.md` |
| `docs/MVP_DEFINITION.md` | Canonical | V1 definition | → `MVP.md` |
| `WEBSITE_CONTENT_MASTER.md` | Canonical | Canonical website content | → `WEBSITE.md` |
| `CLAUDE.md` | Canonical | Agent operating instructions | `CLAUDE.md` |
| `CONTRIBUTING.md` | Canonical | GitHub convention (trim to pointer) | `CONTRIBUTING.md` |
| `firmware/README.md` | Canonical | Scoped firmware build doc | unchanged |
| `firmware/esp-idf/README.md` | Canonical | Scoped ESP-IDF build doc | unchanged |
| `protocol/TRANSPORT.md` | Canonical | Scoped data-plane spec | unchanged |

### Merge — fold into a target (32)

| Current path | → Target | Reason |
|---|---|---|
| `MASTER.md` | `VISION.md` | "Authority" doc = vision/principles; belongs in VISION |
| `CLOUD.md` | `PRODUCT.md` | AI-provider neutrality = product's AI philosophy |
| `MILESTONES.md` | `ROADMAP.md` | Milestones are roadmap content |
| `docs/PRE_HARDWARE_ROADMAP.md` | `ROADMAP.md` | Roadmap subset |
| `docs/APP_MODEL.md` | `ARCHITECTURE.md` | App/services model = architecture |
| `docs/FRAME_ARCHITECTURE.md` | `ARCHITECTURE.md` | Module/frame design = architecture |
| `RUNNING.md` | `DEVELOPMENT.md` | How to run = development |
| `TEST_PLAN.md` | `DEVELOPMENT.md` | How to test = development |
| `IMPLEMENTATION_GAP_REPORT.md` | `DEVELOPMENT.md` (+`VALIDATION.md`) | "What code exists" status |
| `PRE_HARDWARE_EXECUTION_PLAN.md` | `DEVELOPMENT.md` | Pre-hardware task plan |
| `PRE-HARDWARE_STOP_LIST.md` | `DEVELOPMENT.md` | What not to build yet |
| `HARDWARE_ARRIVAL_CHECKLIST.md` | `HARDWARE.md` | Bring-up checklist |
| `docs/HARDWARE_INTEGRATION_PLAN.md` | `HARDWARE.md` | Integration plan |
| `docs/BOARD_RESOURCES.md` | `HARDWARE.md` | Pin/board map |
| `docs/CAMERA_BRINGUP.md` | `HARDWARE.md` | Camera bring-up |
| `docs/AUDIO_BRINGUP.md` | `HARDWARE.md` | Audio bring-up |
| `docs/PROVISIONING_PLAN.md` | `HARDWARE.md` | Provisioning/setup |
| `REFERENCES.md` | `HARDWARE.md` | Datasheets/external refs |
| `ASSUMPTION_REGISTER.md` | `VALIDATION.md` | Assumptions to validate |
| `VALIDATION_BACKLOG.md` | `VALIDATION.md` | Ranked experiments |
| `TRUTH_TABLE.md` | `VALIDATION.md` | Known vs. believed |
| `RISKS.md` | `VALIDATION.md` | Risk register |
| `FAILURE_SCENARIOS.md` | `VALIDATION.md` | Failure modes |
| `FIRST_REAL_DATA_PLAN.md` | `VALIDATION.md` | First evidence to collect |
| `DEMO_METRICS.md` | `VALIDATION.md` | Metrics targets |
| `DEMO_SUCCESS_CRITERIA.md` | `VALIDATION.md` | Pass/fail criteria |
| `DEMO_SCRIPT.md` | `DEMO_AND_LAUNCH.md` | Live demo script |
| `DEMO_VIDEO_PLAN.md` | `DEMO_AND_LAUNCH.md` | Video plan |
| `DEMO_DAY_PLAN.md` | `DEMO_AND_LAUNCH.md` | Demo-day plan |
| `PHOTO_SHOTLIST.md` | `DEMO_AND_LAUNCH.md` | Photo shot list |
| `LAUNCH_READINESS.md` | `DEMO_AND_LAUNCH.md` | Launch readiness |
| `FIRST_PUBLIC_POSTS.md` | `DEMO_AND_LAUNCH.md` | Announcement drafts |

### Archive — moved intact to `archive/` (21) — **executed in this change**

| Current path | Reason |
|---|---|
| `FOUNDER_VISION_ALIGNMENT.md` | One-off analysis; conclusions fold to `VISION.md` |
| `PROJECT_AUDIT.md` | Historical audit (conclusions preserved) |
| `PRODUCT_REVIEW.md` | Historical review |
| `WEBSITE_REVIEW.md` | Historical review of website content |
| `README_REWRITE_PLAN.md` | Working plan; apply then keep for record |
| `OPUS_BRIEF.md` | AI session brief — historical |
| `IDEAS.md` | Idea dump — not canonical |
| `WHY_SUDONIT_WINS.md` | Scenario essay |
| `WHY_SUDONIT_LOSES.md` | Scenario essay |
| `FUTURE_ROADMAP_REVIEW.md` | Roadmap review; conclusions fold to `ROADMAP.md` |
| `FIRST_WEEK_EXPERIENCE.md` | Red-team narrative |
| `ОБЪЯСНЕНИЕ_ДЛЯ_ВЛАДЕЛЬЦА.md` | Personal owner explainer (RU) — not contributor-facing |
| `docs/COMPETITOR_ANALYSIS.md` | Background analysis |
| `docs/DISPLAY_ARCHITECTURE.md` | Display is V2/experimental — out of V1 scope |
| `docs/DISPLAY_BRINGUP_PLAN.md` | Display is V2/experimental |
| `docs/DISPLAY_EVALUATION.md` | Display is V2/experimental |
| `docs/UI_RUNTIME_ARCHITECTURE.md` | Visual UI runtime is V2 (V1 is audio-first) |
| `docs/UI_STATE_MACHINE.md` | Visual UI runtime is V2 |
| `UX_PROTOTYPES/DESIGN_REVIEW.md` | Exploratory UX prototype work |
| `UX_PROTOTYPES/FUTURE_HARDWARE_IMPACT.md` | Exploratory, future-hardware |
| `UX_PROTOTYPES/README.md` | Index for the archived UX folder |

### Delete — pure duplicates (4)

| Current path | Reason | Content already in |
|---|---|---|
| `docs/PRODUCT_VISION.md` | Duplicate of the vision | `VISION.md` / `PRODUCT.md` |
| `TOP_RISKS.md` | Curated subset of the full register | `RISKS.md` → `VALIDATION.md` |
| `QUICK_WINS.md` | Superseded pre-hardware task list | `PRE_HARDWARE_EXECUTION_PLAN.md` → `DEVELOPMENT.md` |
| `CODE_FIRST_BACKLOG.md` | Superseded pre-hardware task list | `PRE_HARDWARE_EXECUTION_PLAN.md` → `DEVELOPMENT.md` |

*(Delete only after the merge targets are built and verified to contain the content.)*

---

## New files created by the merges (4)

`DEVELOPMENT.md`, `HARDWARE.md`, `VALIDATION.md`, `DEMO_AND_LAUNCH.md` — assembled from
their Merge sources above. No new ideas are introduced; they are containers that absorb
existing content and remove the repetition between sources.

---

## Estimated reduction

| Metric | Count |
|---|---|
| **Current project Markdown files** | **71** |
| Canonical (kept active, some renamed) | 14 |
| Merge (folded into a target, husk removed) | 32 |
| Archive (moved intact to `archive/`) | 21 |
| Delete (pure duplicates) | 4 |
| New merged targets created | +4 |
| **Final active documentation files** | **13** |
| **Final non-archive files total** (incl. CLAUDE, CONTRIBUTING, 3 scoped READMEs) | **18** |

Net effect: a contributor's reading surface drops from **71 scattered files** to **7 core
+ 6 supporting** active documents, with everything historical preserved under `archive/`
and nothing lost.

---

## Execution phases

1. **Phase 1 — Archive (lossless) — DONE in this change.** The 21 historical files are
   `git mv`'d into `archive/`. Reversible, zero information loss, immediate sprawl cut.
2. **Phase 2 — Merge (needs review).** Build the 4 new targets by folding the 32 Merge
   sources, de-duplicating as you go; then remove the husks. Content-heavy — do it in
   reviewed batches per target (one PR per target keeps diffs legible).
3. **Phase 3 — Delete.** After Phase 2 verifies the content landed, delete the 4 pure
   duplicates.
4. **Phase 4 — Rename + relink.** Rename `PROJECT_BRIEF_SUDONIT.md`→`PRODUCT.md`,
   `WEBSITE_CONTENT_MASTER.md`→`WEBSITE.md`, `docs/MVP_DEFINITION.md`→`MVP.md`; fix
   cross-links and the README index.

Only Phase 1 is executed here; Phases 2–4 are the reviewable follow-up so no prose is
merged blind.
