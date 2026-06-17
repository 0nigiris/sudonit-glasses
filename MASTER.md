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
