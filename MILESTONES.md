# MILESTONES.md

# Purpose

Track actual progress.

Roadmaps describe plans.

Milestones describe achievements.

---

# Completed

## Project Website

Status:

COMPLETED

Notes:

* Domain purchased
* Website deployed
* Public presence established

Completion Date:

2026

---

# Prototype Milestones

## First Repository

Status:

COMPLETED

Completion Date:

2026-06-17

Description:

Initial project structure created and published to GitHub
(github.com/0nigiris/sudonit-glasses, public, Apache-2.0). Includes the source-
of-truth docs, the shared protocol layer, the phone-brain prototype, the glasses
simulator, and a passing test suite.

---

## First Simulated Loop

Status:

COMPLETED

Completion Date:

2026-06-17

Description:

The full V1 loop runs end-to-end with no hardware:
capture image -> chunked transport (SHA-256 verified) -> AI provider -> text ->
audio. Driven by the glasses simulator against the phone-brain server over TCP
(the Wi-Fi data-plane stand-in). Runs on a deterministic stub provider by
default and on real Claude vision when ANTHROPIC_API_KEY is set.

Note: this is proven in SIMULATION only. The hardware milestones below (camera
capture, ESP32 boot, real phone connection) remain PENDING — when hardware
arrives, only the capture source changes; the rest of the loop is already proven.

---

## First ESP32 Boot

Status:

PENDING

Description:

ESP32 successfully runs project firmware.

---

## First Camera Capture

Status:

PENDING

Description:

Camera successfully captures image.

---

## First Phone Connection

Status:

PENDING

Description:

ESP32 communicates with Android application.

---

## First AI Request

Status:

PENDING

Description:

Image reaches AI provider.

---

## First AI Response

Status:

PENDING

Description:

AI response reaches glasses.

---

## First Audio Output

Status:

PENDING

Description:

Response can be heard.

---

## First Wearable Prototype

Status:

PENDING

Description:

Prototype can be worn on glasses.

---

## First Display Output

Status:

PENDING

Description:

Information visible through optics.

---

# Product Milestones

## Prototype V1

Status:

PENDING

---

## Prototype V2

Status:

PENDING

---

## Custom PCB

Status:

PENDING

---

## First Public Demo

Status:

PENDING

---

## First Community Contributor

Status:

PENDING

---

## First User

Status:

PENDING

---

## First Sale

Status:

PENDING

---

## Product Launch

Status:

PENDING

---

# Rule

Every completed milestone must be marked immediately.

Never underestimate progress.
