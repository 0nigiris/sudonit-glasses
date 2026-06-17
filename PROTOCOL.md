# PROTOCOL.md

# Purpose

This document defines communication between:

* Smart Glasses
* Smartphone
* AI Services

All communication formats must be documented here.

---

# Version

Protocol Version:

1.0

Status:

Prototype

---

# Communication Model

ESP32
↔
Smartphone

Primary Transport:

Bluetooth LE

Future:

Wi-Fi Direct

---

# Message Format

JSON

Example:

{
"type": "ping",
"timestamp": 123456789
}

---

# System Messages

## Ping

Purpose:

Connection test

Example:

{
"type": "ping"
}

Response:

{
"type": "pong"
}

---

# Device Information

Request:

{
"type": "device_info"
}

Response:

{
"device": "Sudonit Glasses",
"firmware": "0.1.0",
"battery": 87
}

---

# Battery Status

Message:

{
"type": "battery_status",
"level": 87
}

---

# Camera Messages

## Capture Request

{
"type": "capture_request"
}

---

## Capture Complete

{
"type": "capture_complete",
"image_id": "123"
}

---

# Image Transfer (Data Plane)

The actual image bytes are moved using the binary data plane defined in
protocol/TRANSPORT.md, not as JSON. An image is bracketed by two control
messages with binary chunk frames in between.

## Image Begin

{
"type": "image_begin",
"image_id": "123",
"size": 184320,
"chunks": 45,
"sha256": "<hex>",
"media_type": "image/jpeg"
}

## Image End

{
"type": "image_end",
"image_id": "123"
}

See TRANSPORT.md for the frame format, chunk layout, and reassembly/verification
rules. Transport: Wi-Fi for image/audio data, BLE for control (see DECISIONS.md).

---

# AI Messages

## Analyze Image

{
"type": "analyze_image",
"image_id": "123"
}

---

## AI Response

{
"type": "ai_response",
"text": "This appears to be a cat."
}

---

# Audio Messages

## Play Audio

{
"type": "play_audio",
"content": "Turn left in 50 meters."
}

---

## Stop Audio

{
"type": "stop_audio"
}

---

# Notification Messages

## Notification

{
"type": "notification",
"title": "Message",
"content": "Hello Alex"
}

---

# Error Messages

{
"type": "error",
"code": 1001,
"message": "Camera unavailable"
}

---

# Future Extensions

Potential additions:

* Display control
* Plugin communication
* Navigation
* Voice assistant
* Local AI support

---

# Rule

All new message types must be documented here before implementation.
