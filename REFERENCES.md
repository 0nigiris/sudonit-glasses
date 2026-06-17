# REFERENCES.md

# Purpose

This file stores useful references, hardware choices, datasheets, repositories, articles and research.

It is the project's memory.

---

# Hardware

## Main Controller

ESP32-S3

Current Target:

ESP32-S3 N16R8

Notes:

* Wi-Fi
* Bluetooth
* Good community support
* AI companion architecture

---

## Camera

Current Target:

OV5640

Reasons:

* Better image quality
* Good ESP32 compatibility
* Suitable for AI image analysis

Alternative:

OV2640

---

## Microphone

Preferred:

ICS43434

Alternative:

INMP441

Interface:

I2S

---

## Audio

Current Direction:

Open-ear speaker

Reason:

* Smaller than bone conduction
* Better audio quality
* Easier integration

Amplifier:

MAX98357A

---

## Battery

Current Experiments:

502080 1000mAh

Potential Layout:

Left arm:
Battery

Right arm:
Battery + Electronics

Benefits:

* Weight balancing
* Longer runtime

---

## Charging

TP4056 USB-C

Requirements:

* Battery protection
* USB-C connector

---

# Software

## Smartphone Companion

Platform:

Android

Responsibilities:

* AI communication
* Cloud access
* Notifications
* Settings
* Storage

---

## Firmware

Platform:

ESP32-S3

Responsibilities:

* Camera control
* Audio
* Communication
* Sensors
* Display

---

# Future Research

## Display

Research Required:

* SPI displays
* OLED microdisplays
* HDMI microdisplays
* AR optics

Status:

OPEN

---

## Optics

Research Required:

* Prism systems
* Birdbath optics
* Waveguides

Status:

OPEN

---

# Repositories

Useful repositories should be added here.

Template:

Name:
URL:
Purpose:
Notes:

---

# Articles

Useful articles should be added here.

Template:

Title:
URL:
Summary:

---

# Lessons Learned

Important discoveries should always be documented here.

Reason:

Future Alex will forget.
