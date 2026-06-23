<div align="center">

# 👓 Sudonit Smart Glasses

**Turn any glasses into smart glasses.**

An open, modular smart-glasses platform: a clip-on camera + speaker module that
sees, listens, and talks back — with the heavy AI running on the phone in your
pocket, not on your face.

[![License: Apache 2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Platform: ESP32-S3](https://img.shields.io/badge/platform-ESP32--S3-e7352c.svg)](HARDWARE.md)
[![Phase: Prototype](https://img.shields.io/badge/phase-prototype-orange.svg)](#-project-status)
[![Audio-first V1](https://img.shields.io/badge/V1-audio--first-success.svg)](#-how-it-works)

</div>

---

## Why Sudonit

Most smart glasses force you to buy *their* frame, lock you into *their* cloud,
and hide *their* code. Sudonit takes the opposite bet:

- **Clip-on, not replace** — a small module attaches to glasses you already own.
- **Phone-brokered AI** — the glasses capture and play; your phone does the
  expensive inference and holds the network. The wearable stays cheap, light,
  and low-power.
- **Open & understandable** — a small team should be able to read the whole
  stack. No black boxes.

---

## 🔍 How it works

V1 is **audio-first**: there is no display. You point, it answers out loud.

```
   ┌─────────────┐        Wi-Fi / TCP        ┌──────────────┐      API      ┌─────────┐
   │  Glasses    │  ── image_begin + chunks ─▶│    Phone     │ ───────────▶ │   AI    │
   │  (ESP32-S3) │                            │  (the brain) │              │ (Claude)│
   │             │  ◀── audio_begin + chunks ─│              │ ◀─────────── │         │
   │ camera 📷   │        (PCM speech)        │  pipeline    │              └─────────┘
   │ speaker 🔊  │                            │  + framing   │
   └─────────────┘                            └──────────────┘
```

1. The glasses capture an image and stream it to the phone in fixed-size chunks
   (length-prefixed frames, SHA-256 verified).
2. The phone runs the image through its **pipeline** (AI provider → text → TTS).
3. The phone streams **raw PCM speech** back; the glasses play it on the I²S amp.

The wire protocol is deliberately tiny so the ESP32 can implement it in C with a
fixed-width header and no JSON parser in the hot path — see **[PROTOCOL.md](PROTOCOL.md)**
and **[protocol/TRANSPORT.md](protocol/TRANSPORT.md)**.

---

## 🧩 Hardware target

| Part        | Component                      | Role                          |
|-------------|--------------------------------|-------------------------------|
| MCU         | ESP32-S3-WROOM-1 (N16R8)       | 16 MB flash · 8 MB PSRAM · Wi-Fi |
| Camera      | OV5640 (DVP)                   | image capture                 |
| Audio out   | MAX98357A (I²S)                | speech playback               |
| Form factor | clip-on module                 | attaches to existing glasses  |

Details and bring-up notes: **[HARDWARE.md](HARDWARE.md)**.

---

## 📂 Repository layout

```
firmware/     ESP32-S3 firmware — HAL interfaces + swappable backends (mock / host / esp32)
protocol/     Wire framing + control/transfer messages (Python reference, mirrors the C)
phone/        Phone-side "brain": TCP server, AI pipeline, image + audio handling
eval/         Offline evaluation harness for the AI pipeline
benchmarks/   Image/latency benchmark inputs and tooling
tools/        Dev utilities
tests/        Host-side Python tests
.github/      CI (host build + ctest + pytest + lint, and the ESP-IDF build)
```

The firmware uses a **HAL pattern**: hardware-facing interfaces
(`firmware/include/sudonit/hal/*.h`) with backends selected at build time —
`mock`/`host` for CI on a laptop, `esp32` for real silicon. That is why the whole
data path can be tested today, before the hardware lands.

---

## 🚀 Quick start (host, no hardware needed)

Everything but the on-silicon drivers builds and tests on a normal machine.

```bash
# C firmware (host backends) + Python suite: configure, build, ctest, pytest
make ci

# just the linters (ruff + cppcheck)
make lint

# run the phone-side server (listens on 127.0.0.1:8765)
python -m phone.server
```

See **[DEVELOPMENT.md](DEVELOPMENT.md)** for the full developer workflow and
**[VALIDATION.md](VALIDATION.md)** for what is proven on host vs. what waits for
hardware.

---

## 📊 Project status

**Phase: Prototype — pre-hardware bring-up.**

The full software path (framing, transfer, reassembly, pipeline, host firmware
backends) is implemented and green in CI. What remains is gated on physical
silicon: real camera/audio drivers, Wi-Fi power stability, and first real
latency/battery numbers.

| Stage | Focus                          | State |
|-------|--------------------------------|-------|
| 0     | Architecture & protocol        | ✅ done |
| 1     | Image capture → link transfer  | ✅ host-proven |
| 2     | AI pipeline integration        | ✅ host-proven |
| 3     | Audio interaction (round trip) | ✅ host-proven |
| 4     | Wearable prototype on silicon  | ⛔ needs hardware |
| 5     | Display integration            | 🔭 future |

---

## 🤝 Contributing

Contributions are welcome — especially **embedded (ESP32) development**, **mobile
development**, **hardware engineering**, and **UX**. Start with
**[CONTRIBUTING.md](CONTRIBUTING.md)**; architectural reasoning lives in
**[ARCHITECTURE.md](ARCHITECTURE.md)** and **[DECISIONS.md](DECISIONS.md)**.

---

## 📜 License

Apache License 2.0 — see **[LICENSE](LICENSE)**.

<div align="center">
<sub>Build a smart-glasses platform that users can truly own, modify, and understand.</sub>
</div>
