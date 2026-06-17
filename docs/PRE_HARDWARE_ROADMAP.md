# PRE_HARDWARE_ROADMAP.md

Every subsystem that can be built, tested, and documented **before** any ESP32
hardware arrives. Objective: when hardware lands, the only major unknowns are
camera capture, audio-playback hardware, microphone hardware, power consumption,
and physical assembly. Everything else already exists.

Columns:
- **Why no hardware** — why it's fully developable/testable on a dev machine.
- **Time saved** — rough estimate of hardware-bring-up time this removes later.
- **Risk reduction** — how much integration/schedule risk it retires.
- **Priority** — Critical / High / Medium / Low.

Status legend: ✅ done · 🟡 in progress · ⬜ not started.

---

## Already completed (Milestone 0)

| Subsystem | Status |
|-----------|--------|
| Protocol framing + chunked image transfer (Python reference) | ✅ |
| Vendor-neutral AI abstraction (Claude + stub) | ✅ |
| Phone-brain pipeline + TCP server | ✅ |
| Glasses simulator (host-side) | ✅ |
| Test suite (protocol + pipeline) | ✅ |
| License, run guide, contributor docs | ✅ |

---

## Firmware-side subsystems

| Subsystem | Description | Why no hardware | Time saved | Risk reduction | Priority |
|-----------|-------------|-----------------|------------|----------------|----------|
| **Firmware HAL + mock drivers** | Interfaces for camera/audio/mic/battery/transport + mock implementations; the device app runs against mocks on a host build. | Mocks return canned data; whole device loop compiles and runs with gcc, no ESP-IDF/silicon. | **High** — turns each peripheral into a one-file driver swap. | **Critical** — defines the entire hardware boundary up front; localizes every future failure to one driver. | **Critical** |
| ESP-IDF protocol component (C) | Port framing + chunked transfer to C, host-buildable, interop-tested against the Python phone-brain. | Pure logic over a socket; runs natively, no peripherals. | High — firmware speaks the proven wire format on day one. | High — kills protocol drift between firmware and phone. | High |
| Firmware config system | Compile-time + NVS-backed config (Wi-Fi creds, server addr, IDs) behind a `config.h` interface, mock-backed on host. | Key/value abstraction; NVS mocked in-memory on host. | Medium | Medium — avoids hardcoded values during bring-up. | High |
| Logging system | Leveled `SD_LOG` macros mapping to ESP-IDF log on device, stderr on host. | Pure macros/printf. | Medium — debugging ready before first boot. | Medium | High |
| Error-handling model | `sd_err_t` codes + conventions across HAL/protocol. | Pure C. | Medium — clean failure paths from the start. | High — every HAL call returns a checkable status. | High |
| Device state machine | The capture→transport→await-response loop as an explicit state machine over the HAL. | Runs against mocks. | Medium | Medium | Medium |
| OTA update architecture (doc + seam) | Design doc + a `hal/ota.h` seam; partition plan. | Design + interface only; no flashing needed yet. | Medium (later) | Medium — avoids a painful retrofit. | Medium |
| Power-management strategy (doc) | Sleep/wake plan, radio duty-cycling, capture-then-transmit vs concurrent. | Analysis + budget doc; validated against HW later. | Medium | High — battery is priority #2 and unmodeled. | Medium |

## Phone / Android subsystems

| Subsystem | Description | Why no hardware | Time saved | Risk reduction | Priority |
|-----------|-------------|-----------------|------------|----------------|----------|
| AI abstraction improvements | Provider registry, prompt config, timeout/retry, cost guard. | Pure phone-side logic; stub-testable. | Medium | Medium | High |
| Android app architecture (doc) | Module layout (core-transport / core-ai / feature-capture), DI, screens. | Design doc; no device needed. | High (later) | High — sets the app up to mirror the proven pipeline. | High |
| Android prototype (against simulator) | Kotlin app that speaks the protocol to the phone-brain/simulator over TCP. | Talks to the host server; no glasses needed (emulator OK for non-BLE). | High | High — proves the real companion against the contract. | High |
| Settings system | Provider/model, server address, privacy toggles. | Pure app/config logic. | Low | Medium | Medium |
| Local storage | Ephemeral image/result handling per PRIVACY rules. | Pure app logic. | Low | Medium (privacy) | Medium |
| Pairing system (design) | Device discovery + trust model for BLE control link. | Design + state machine; testable with simulated peers. | Medium | High — security-sensitive, better designed early. | Medium |
| Authentication / security model (doc) | Threat model, pairing auth, data-plane encryption plan. | Analysis/design. | Medium | High | High |

## Tooling / cross-cutting subsystems

| Subsystem | Description | Why no hardware | Time saved | Risk reduction | Priority |
|-----------|-------------|-----------------|------------|----------------|----------|
| Host build + CI | CMake host build for firmware + GitHub Actions running pytest + the C tests. | Build/test infra. | Medium | High — every change stays green automatically. | High |
| Protocol JSON schemas | Schemas for every message; validate in CI; basis for codegen. | Pure data contract. | Medium | High — single source of truth for both ends. | Medium |
| Diagnostics / self-test tool | A "device doctor" that exercises each HAL interface and reports pass/fail. | Runs against mocks now, real drivers later. | Medium | High — turns bring-up into a checklist. | Medium |
| Hardware simulation (Wokwi) | Run firmware in Wokwi for CI smoke tests. | Emulator. | Medium | Medium | Low |
| Telemetry architecture (doc) | What we measure (latency, battery, errors) and where it goes. | Design. | Low | Medium | Low |

---

## The single highest-value pick

**Firmware HAL + mock drivers + host build + tests.**

Rationale: the stated objective is *reduce future hardware integration work as
much as possible*, and the success criterion is *the only remaining unknowns are
the peripherals themselves*. The HAL is the literal boundary between "the
peripherals" and "everything else." Building it now, with mock drivers and a
host build, means:

- every other firmware subsystem (config, logging, state machine, protocol) is
  written against stable interfaces immediately;
- when a peripheral arrives, integration is implementing one `*_esp.c` behind an
  existing header — exactly the "camera capture / audio / mic" unknowns the
  success criteria allow, and nothing more;
- the whole device loop runs and is tested on a laptop today.

It is broader than protocol work (addressing the explicit "don't focus only on
protocol" guidance) and it unlocks the rest of the firmware roadmap. Implemented
first.
