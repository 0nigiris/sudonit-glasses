# CLOUD.md

# Purpose

This document defines all cloud-related architecture decisions.

The glasses themselves are not responsible for AI inference.

AI workloads are delegated to external services.

---

# Cloud Strategy

Principle:

Use the smartphone as the cloud gateway.

Architecture:

Glasses
↓
Phone
↓
Cloud Provider
↓
Phone
↓
Glasses

---

# Supported Providers

## OpenAI

Potential Uses:

* Image analysis
* GPT conversations
* Translation

Status:

Supported

---

## Anthropic

Potential Uses:

* Reasoning
* Planning
* Long-form responses

Status:

Supported

---

## Gemini

Potential Uses:

* Android integration
* Multimodal processing

Status:

Future

---

## Local Models

Potential Uses:

* Privacy
* Offline operation

Status:

Research

---

# Image Processing Pipeline

Capture Image
↓
Compress Image
↓
Send To Phone
↓
Send To AI Provider
↓
Receive Response
↓
Display Response

---

# Voice Processing Pipeline

User Speech
↓
Phone
↓
Speech To Text
↓
AI Provider
↓
Response
↓
Text To Speech
↓
User

---

# Privacy Rules

User owns all data.

No unnecessary cloud storage.

No permanent image retention.

No selling user data.

---

# Cost Strategy

Goal:

Keep cloud costs low.

Techniques:

* Request caching
* Image compression
* Local preprocessing
* Provider selection

---

# Future Research

* Multi-provider support
* Local AI fallback
* Hybrid cloud mode
* Offline operation

---

# Important Rule

Cloud providers are replaceable.

The system must never depend on a single AI vendor.
