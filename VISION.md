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
