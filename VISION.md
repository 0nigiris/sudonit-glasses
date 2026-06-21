# Vision.md

# Project Name

Sudonit Smart Glasses

# Mission

Create a modular, open, privacy-respecting smart glasses platform that can be attached to existing glasses rather than replacing them.

Users should not be forced to buy a new frame. Instead, they can transform their favorite glasses into smart glasses using a lightweight attachable module.

# Core Principles

1. Autonomy first. (canonical)

Everything that can reasonably run on the glasses runs on the glasses. External
devices — phone, server, AI provider — extend the system; they do not replace it.
Sudonit is a standalone device that becomes more powerful when connected, never a thin
client that is useless without a phone. We optimize for long-term device independence
over minimal firmware. See `docs/DISPLAY_ARCHITECTURE.md` §0.

2. Open ecosystem.

Users should be able to modify, extend and customize the software.

3. Privacy first.

User data belongs to the user.

4. Modularity.

Every component should be replaceable.

5. Repairability.

Users should be able to repair most hardware components.

6. Simplicity.

The system must remain understandable and maintainable — simple, but not dependent.

# Long-Term Goal

Create a product that can compete with Ray-Ban Meta while remaining open and highly customizable.

# Version 1 Goals

* Camera support
* Voice interaction
* Smartphone integration
* GPT integration
* Notifications
* Battery monitoring

# Future Goals

* Display
* Navigation
* Real-time translation
* Object recognition
* Open application ecosystem
* Custom hardware
* Custom operating system

# Non-Goals

* Replacing smartphones
* Running large AI models or heavy cloud processing locally
* Depending on the phone for the device's own basic functions

Note: device autonomy is a goal (see Core Principle 1). The non-goal is *on-device
heavy computation* (AI, routing, cloud), not device independence — the glasses must
run their own local functions without a phone, while leaving the heavy lifting to it.


---

<!-- ===== consolidated from: MASTER.md ===== -->

# MASTER.md

# Project Authority

This file is the highest-level project specification.

When conflicts occur:

MASTER.md overrides all other documents.

# Product Name

Sudonit Smart Glasses

# Product Type

Modular smart-glasses attachment system.

Users should be able to attach the module to existing glasses whenever possible.

# Primary User

A technically curious user who wants AI assistance without constantly using a smartphone.

# Core Hardware

Current prototype target:

* ESP32-S3
* OV5640 Camera
* Battery system
* I2S microphone
* Audio output
* Smartphone companion application

# Architecture Philosophy

The smartphone performs heavy computation.

The glasses perform:

* capture
* communication
* display
* audio

The smartphone performs:

* AI inference
* networking
* cloud communication
* storage

# Open Source Philosophy

Software should be open whenever possible.

Security-sensitive components may remain closed.

# Design Requirements

The system must:

* remain lightweight
* remain modular
* remain repairable
* remain power efficient

The system must not:

* depend on proprietary ecosystems
* lock users into subscriptions
* require vendor-controlled hardware

# Development Priorities

Priority 1:

Reliable functionality.

Priority 2:

Battery life.

Priority 3:

User comfort.

Priority 4:

Performance.

Priority 5:

Features.

# Rule

Never add complexity unless it solves a real user problem.

# Current Goal

Build a functioning prototype that can:

1. Capture an image.
2. Send it to a smartphone.
3. Ask an AI model for analysis.
4. Return the answer.
5. Present the answer to the user.
