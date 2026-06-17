# TRANSPORT.md — Sudonit data-plane specification

Status: v1 (prototype)
Companion to: PROTOCOL.md (control messages)

---

## Why this document exists

PROTOCOL.md defines the *control plane*: small JSON messages like `ping`,
`battery_status`, `capture_request`. It did **not** define how the product's
actual payload — a camera image (and later, audio) — crosses the link. That was
the single biggest gap in the spec: the core loop is "send a photo to the
phone," and there was no defined way to send a photo.

This document defines that data plane. It is implemented and tested in
`protocol/framing.py` and `protocol/messages.py`, and exercised end-to-end by
the simulator + phone server.

---

## Transport choice: Wi-Fi for bulk, BLE for control

The original architecture named **BLE as the primary transport**. That does not
survive contact with the numbers:

- Realistic BLE application throughput is ~0.1–1.4 Mbps.
- An OV5640 JPEG worth analysing is ~150–600 KB.
- => **1–6+ seconds of transfer per image**, before AI latency.

The product promise is that it "feels invisible" (OPUS_BRIEF.md). BLE-for-images
breaks that. So the v1 design is:

| Plane    | Carries                                  | Transport (v1)        |
|----------|------------------------------------------|-----------------------|
| Control  | ping, battery, capture trigger, status   | BLE (low-power, ideal for small msgs) |
| Data     | images, audio                            | **Wi-Fi** (TCP/IP)    |

The reference implementation runs the **data plane over a TCP socket** as a
faithful stand-in for Wi-Fi. The framing below is transport-agnostic: the ESP32
firmware emits the identical frames over its Wi-Fi link. See
`DECISIONS.md` (2026-06-17, "Transport split").

---

## Frame format

Every message on the data plane is a length-prefixed frame:

```
+--------+------------------+------------------------+
| kind   | length (uint32)  | payload (length bytes) |
| 1 byte | big-endian       |                        |
+--------+------------------+------------------------+
```

- `kind = 'J'` (0x4A): payload is a UTF-8 JSON control message.
- `kind = 'B'` (0x42): payload is a binary chunk (see below).

Fixed-width, length-prefixed, no JSON parser required to find frame
boundaries — chosen so the C firmware side is trivial and robust. Frames larger
than 8 MiB are rejected (`MAX_FRAME_BYTES`) so a corrupt header can't trigger a
huge allocation.

A binary chunk payload is itself:

```
+----------------+-------------------+
| seq (uint32 BE)| chunk data        |
+----------------+-------------------+
```

---

## Image transfer

An image is sent as a JSON `image_begin`, then N binary chunks, then a JSON
`image_end`:

```
glasses -> phone:  {"type":"image_begin","image_id":"...","size":N,
                    "chunks":K,"sha256":"...","media_type":"image/jpeg"}
glasses -> phone:  [B] seq=0 | <chunk 0 bytes>
glasses -> phone:  [B] seq=1 | <chunk 1 bytes>
                   ...
glasses -> phone:  {"type":"image_end","image_id":"..."}
```

The receiver:
1. allocates a reassembly buffer on `image_begin`,
2. places each chunk by `seq`,
3. on `image_end`, concatenates in `seq` order and verifies **both** the byte
   count and the SHA-256 from `image_begin`.

A size or hash mismatch is a `ProtocolError` — the image is discarded rather
than analysed corrupt. Default chunk size is 4096 bytes (`DEFAULT_CHUNK_SIZE`),
tunable per transport.

### Reply

After analysis the phone sends two control frames back:

```
phone -> glasses:  {"type":"ai_response","text":"..."}   # for a display/log
phone -> glasses:  {"type":"play_audio","content":"..."}  # speak it
```

---

## Not yet specified (intentionally deferred)

- **Resume / partial-retry** of a failed transfer (v1 re-sends the whole image).
- **Audio uplink** (mic → phone) framing — same chunk mechanism, different
  `*_begin`/`*_end` types; add when Phase 3 starts.
- **Encryption** — the security model (PROTOCOL/ARCHITECTURE) calls for pairing +
  encryption; the data plane will run inside that, unspecified here.

Every new message type added here must also be registered in
`protocol/messages.py::KNOWN_TYPES` and documented in PROTOCOL.md before
implementation (PROTOCOL.md rule).
