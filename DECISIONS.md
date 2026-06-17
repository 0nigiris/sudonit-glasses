# DECISIONS.md

# Purpose

This document records important project decisions.

The goal is to preserve reasoning and prevent repeated debates.

---

# Format

Date:

Decision:

Reason:

Alternatives:

Tradeoffs:

Status:

---

# 2026-06-17

Decision:

Use ESP32-S3 as the primary prototype platform.

Reason:

* Low cost
* Widely available
* Good community support
* Camera support
* Bluetooth
* Wi-Fi

Alternatives:

* Raspberry Pi Zero 2 W
* Raspberry Pi CM4
* Raspberry Pi CM5

Tradeoffs:

Pros:

* Cheap
* Efficient
* Small

Cons:

* Limited performance
* Limited graphics capability

Status:

Accepted

---

# 2026-06-17

Decision:

Use smartphone as primary compute device.

Reason:

Modern smartphones already contain:

* CPU
* GPU
* Internet
* AI access
* Battery

Using them reduces hardware complexity.

Alternatives:

* Fully standalone glasses
* Raspberry Pi architecture

Tradeoffs:

Pros:

* Lower cost
* Lower power consumption
* Faster development

Cons:

* Requires smartphone

Status:

Accepted

---

# 2026-06-17

Decision:

Prefer open-ear speakers over bone conduction.

Reason:

* Smaller
* Cheaper
* Better audio quality
* Easier integration

Alternatives:

* Bone conduction

Tradeoffs:

Pros:

* Simpler design

Cons:

* Slight audio leakage

Status:

Accepted

---

---

# 2026-06-17

Decision:

Transport split — Wi-Fi for the data plane (images, audio), BLE for the
control plane (ping, battery, capture trigger, status).

Reason:

The prior architecture named BLE as the primary transport for everything.
Realistic BLE application throughput is ~0.1–1.4 Mbps; an OV5640 JPEG worth
analysing is ~150–600 KB. That is 1–6+ seconds of transfer per image before
any AI latency, which breaks the "feels invisible" product promise. Wi-Fi on
the ESP32-S3 moves the same image in a fraction of the time. BLE remains the
right choice for low-power presence and small control messages.

Alternatives:

* BLE primary for everything (original plan) — rejected on bandwidth.
* Wi-Fi for everything — rejected; wastes power for tiny control messages and
  loses BLE's low-power standby.

Tradeoffs:

Pros:

* Image/audio transfer fast enough for a responsive UX.
* Keeps BLE's power efficiency for the always-on control link.

Cons:

* Two radios/links to manage instead of one (added firmware complexity).
* Wi-Fi association adds power draw during active transfers (acceptable; bounded
  to capture events).

Status:

Accepted. Implemented in protocol/ (framing + chunked image transfer), proven
end-to-end by the simulator + phone-brain over TCP (Wi-Fi stand-in). See
protocol/TRANSPORT.md.

---

# Future Decisions

All major technical decisions should be documented here.

No exceptions.
