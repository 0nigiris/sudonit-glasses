# ARCHITECTURE.md

# System Architecture

This document describes the overall architecture of the Sudonit Smart Glasses platform.

---

# Governing Principle (canonical)

**Everything that can reasonably run on the glasses runs on the glasses. External
devices extend the system; they do not replace it.**

Sudonit is an autonomous device that becomes *more* powerful when connected to a
phone, server, or AI provider — never a thin client that is useless without one. The
glasses own and persist their own clock, calendar, settings, battery management,
notifications, camera control, and UI rendering, and remain functional with no phone
present. The phone/server *extends* the device with capabilities that genuinely exceed
an ESP32-class MCU (AI inference, navigation routing, cloud sync, large-scale
processing). This principle is canonical and overrides any older wording in this file.

See `docs/DISPLAY_ARCHITECTURE.md` §0 for the detailed model.

---

# High-Level Overview

+------------------+
| User |
+------------------+
|
v
+------------------+
| Smart Glasses |
+------------------+
|
v
+------------------+
| Smartphone |
+------------------+
|
v
+------------------+
| AI Provider |
+------------------+

The Smartphone and AI Provider tiers are **optional extensions**. The glasses are
the product; remove the lower tiers and the device still runs its local functions.

---

# Smart Glasses Responsibilities

The glasses are responsible for (and own the state for):

* Capturing images
* Playing audio
* Rendering the local UI / display
* Receiving user input and navigating local screens
* Clock and timekeeping
* Calendar / agenda (local store, synced)
* Settings (device-owned, persisted on device)
* Battery management
* Notifications (received, stored, viewed, dismissed on device)
* Camera controls
* Local storage of the above device-owned state
* Communicating with the smartphone (when present)

The glasses are NOT responsible for (these genuinely exceed the MCU and are the
phone's job to *provide*, not the glasses' to depend on):

* AI inference
* Navigation route computation
* Cloud communication
* Large-scale / heavy processing

---

# Smartphone Responsibilities

The smartphone is an **extension**, not the primary device. It provides capabilities
that exceed the glasses and enriches device-owned state by syncing — it is not the
source of truth for the device's own data.

Responsibilities:

* AI communication
* Navigation routing
* Networking / internet egress
* Cloud sync and backup (including a backup copy of device-owned settings)
* Large-scale processing and rich integrations
* Authentication
* Plugin management

The device owns its settings and local data; the phone may back them up and sync
them, but the glasses remain authoritative and functional without it.

---

# AI Provider Responsibilities

Examples:

* OpenAI
* Anthropic
* Local models

Responsibilities:

* Image analysis
* Language processing
* Reasoning
* Translation

---

# Communication Model

ESP32
↔
Smartphone

Communication methods:

Preferred:

* Bluetooth LE

Alternative:

* Wi-Fi Direct

Future:

* Hybrid Bluetooth + Wi-Fi

---

# Image Pipeline

User
↓
Capture
↓
Camera
↓
ESP32
↓
Phone
↓
AI
↓
Phone
↓
ESP32
↓
User

---

# Audio Pipeline

User Speech
↓
Microphone
↓
ESP32
↓
Phone
↓
AI
↓
Phone
↓
ESP32
↓
Speaker

---

# Display Pipeline

The phone sends **semantic content** (a notification payload, an AI answer, a route,
calendar events to sync) — not pixels. The glasses render their own UI locally from
that content and from local state. A raw-bitmap path exists only as an escape hatch
for content the device genuinely cannot render itself.

Local state / content
↓
(phone supplies content & sync — optional)
↓
ESP32 UI runtime (layout + widgets + fonts)
↓
Framebuffer
↓
Display
↓
User

The display also renders purely local screens (clock, settings, battery, calendar)
with no phone involved.

---

# Security Model

User Device
↔
Trusted Phone

Requirements:

* Device pairing
* Authentication
* Encryption
* Local trust model

---

# Future Architecture

Potential future variants:

* Raspberry Pi Edition
* Linux Edition
* Pro Edition

The software architecture should remain compatible whenever possible.

---

# Important Principle

Only what genuinely exceeds the device — AI inference, navigation routing, cloud, and
large-scale processing — belongs on the smartphone. Everything else runs on the
glasses.

The glasses should remain efficient and maintainable, but **autonomy comes before
minimal firmware**: we accept more on-device code in exchange for a device that stands
on its own. "Lightweight" means efficient, not dependent.
