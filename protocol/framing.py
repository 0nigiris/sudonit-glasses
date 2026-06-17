"""Wire framing for the Sudonit data plane.

This is the layer PROTOCOL.md was missing: a defined way to move *bytes*
(JPEG/PNG images, later audio) across the link, not just small JSON control
messages. It is transport-agnostic — here it rides a TCP socket (a stand-in
for the Wi-Fi data plane; see DECISIONS / TRANSPORT.md), but the exact same
frame layout is what the ESP32 firmware will emit over its chosen transport.

Frame layout on the wire:

    +--------+------------------+-----------------------+
    | kind   | length (4, BE)   | payload (length bytes)|
    | 1 byte | uint32           |                       |
    +--------+------------------+-----------------------+

    kind = b'J'  -> payload is a UTF-8 JSON control message
    kind = b'B'  -> payload is a binary chunk: [seq:uint32 BE][data...]

Keeping the framing this small and explicit matters: an ESP32 has to implement
the other end of this in C with a fixed-size header and no JSON parser in the
hot path, so the header is fixed-width and length-prefixed by design.
"""

from __future__ import annotations

import socket
import struct

KIND_JSON = b"J"
KIND_BINARY = b"B"

_HEADER = struct.Struct(">cI")  # kind (1 byte), length (uint32 big-endian)

# Guardrail: refuse absurd frame sizes so a corrupt/hostile header can't make
# the receiver try to allocate gigabytes. A single JPEG frame is well under this.
MAX_FRAME_BYTES = 8 * 1024 * 1024


class ProtocolError(Exception):
    """Raised when a frame is malformed or the peer closed mid-frame."""


def send_frame(sock: socket.socket, kind: bytes, payload: bytes) -> None:
    """Send one framed message. `kind` must be KIND_JSON or KIND_BINARY."""
    if kind not in (KIND_JSON, KIND_BINARY):
        raise ProtocolError(f"unknown frame kind: {kind!r}")
    if len(payload) > MAX_FRAME_BYTES:
        raise ProtocolError(f"frame too large: {len(payload)} > {MAX_FRAME_BYTES}")
    sock.sendall(_HEADER.pack(kind, len(payload)) + payload)


def recv_frame(sock: socket.socket) -> tuple[bytes, bytes]:
    """Receive exactly one frame. Returns (kind, payload).

    Raises ProtocolError on a short read (peer closed) or an oversized frame.
    """
    header = _recv_exactly(sock, _HEADER.size)
    kind, length = _HEADER.unpack(header)
    if length > MAX_FRAME_BYTES:
        raise ProtocolError(f"declared frame too large: {length} > {MAX_FRAME_BYTES}")
    payload = _recv_exactly(sock, length) if length else b""
    return kind, payload


def _recv_exactly(sock: socket.socket, n: int) -> bytes:
    """Read exactly n bytes or raise — TCP recv() may return fewer than asked."""
    chunks: list[bytes] = []
    remaining = n
    while remaining:
        chunk = sock.recv(remaining)
        if not chunk:
            raise ProtocolError("connection closed mid-frame")
        chunks.append(chunk)
        remaining -= len(chunk)
    return b"".join(chunks)
