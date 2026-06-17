# ARCHITECTURE.md

# System Architecture

This document describes the overall architecture of the Sudonit Smart Glasses platform.

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

---

# Smart Glasses Responsibilities

The glasses are responsible for:

* Capturing images
* Playing audio
* Displaying information
* Receiving user input
* Communicating with smartphone

The glasses are NOT responsible for:

* AI inference
* Cloud communication
* Long-term storage

---

# Smartphone Responsibilities

The smartphone acts as the primary computing device.

Responsibilities:

* AI communication
* Networking
* Data storage
* User settings
* Authentication
* Plugin management

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

Notification
↓
Phone
↓
ESP32
↓
Display
↓
User

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

Heavy computation belongs on the smartphone.

The glasses should remain lightweight, efficient and simple.
