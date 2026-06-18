"""Control-message builders and the chunked image-transfer protocol.

Control messages are small JSON objects (the message types already named in
PROTOCOL.md: ping/pong, device_info, battery_status, capture_*, ai_response,
play_audio, error). They ride KIND_JSON frames.

Image data rides KIND_BINARY frames, bracketed by two control messages:

    -> image_begin   {image_id, size, chunks, sha256, media_type}
    -> [binary frame: seq=0 | data]
    -> [binary frame: seq=1 | data]
       ...
    -> image_end     {image_id}

The receiver reassembles the chunks in `seq` order and verifies the SHA-256
before handing the image to the pipeline. This is the concrete answer to the
"how does an image actually cross the link" gap.
"""

from __future__ import annotations

import hashlib
import json
import socket
import struct
from dataclasses import dataclass

from . import framing

# Default chunk size for image transfer. Sized to be comfortable for a single
# BLE-MTU-style write while still being efficient over Wi-Fi/TCP; tunable.
DEFAULT_CHUNK_SIZE = 4096

_SEQ = struct.Struct(">I")  # per-chunk sequence number

# Control message types that are part of protocol v1. Anything else is rejected
# so firmware and app can't silently drift (PROTOCOL.md: "all new message types
# must be documented here before implementation").
KNOWN_TYPES = frozenset(
    {
        "ping",
        "pong",
        "device_info",
        "battery_status",
        "capture_request",
        "image_begin",
        "image_end",
        "analyze_image",
        "ai_response",
        "audio_begin",
        "audio_end",
        "play_audio",
        "stop_audio",
        "notification",
        "error",
    }
)


def encode_control(msg: dict) -> bytes:
    """Serialize a control message to canonical JSON bytes.

    sort_keys keeps the bytes deterministic — useful for tests and for any
    future signing/caching that depends on a stable serialization.
    """
    msg_type = msg.get("type")
    if msg_type not in KNOWN_TYPES:
        raise framing.ProtocolError(f"unknown control message type: {msg_type!r}")
    return json.dumps(msg, sort_keys=True, separators=(",", ":")).encode("utf-8")


def decode_control(payload: bytes) -> dict:
    msg = json.loads(payload.decode("utf-8"))
    if not isinstance(msg, dict) or "type" not in msg:
        raise framing.ProtocolError("control message missing 'type'")
    return msg


def send_control(sock: socket.socket, msg: dict) -> None:
    framing.send_frame(sock, framing.KIND_JSON, encode_control(msg))


# --- Image transfer -------------------------------------------------------


def send_image(
    sock: socket.socket,
    image_id: str,
    data: bytes,
    media_type: str = "image/png",
    chunk_size: int = DEFAULT_CHUNK_SIZE,
) -> None:
    """Send an image as image_begin + N binary chunks + image_end.

    This mirrors what the ESP32 will do after a capture: announce the blob,
    stream it in fixed-size chunks, then close it out.
    """
    chunks = max(1, (len(data) + chunk_size - 1) // chunk_size)
    send_control(
        sock,
        {
            "type": "image_begin",
            "image_id": image_id,
            "size": len(data),
            "chunks": chunks,
            "sha256": hashlib.sha256(data).hexdigest(),
            "media_type": media_type,
        },
    )
    for seq in range(chunks):
        start = seq * chunk_size
        chunk = data[start : start + chunk_size]
        framing.send_frame(sock, framing.KIND_BINARY, _SEQ.pack(seq) + chunk)
    send_control(sock, {"type": "image_end", "image_id": image_id})


# --- Audio transfer (downlink: phone -> glasses) --------------------------
#
# The mirror image of the uplink: the phone renders speech to PCM and streams it
# back for the glasses to play. It reuses the exact same proven framing as the
# image path (begin / binary chunks / end + SHA-256), so the device's receive
# side is symmetric with its send side. The payload is raw 16-bit little-endian
# PCM — exactly what the firmware's sd_audio_play_pcm() consumes — so no codec is
# needed in the hot path. (Replaces sending the answer as text in `play_audio`,
# which the device could not actually play.)


def send_audio(
    sock: socket.socket,
    audio_id: str,
    pcm: bytes,
    sample_rate: int,
    channels: int,
    audio_format: str = "pcm_s16le",
    chunk_size: int = DEFAULT_CHUNK_SIZE,
) -> None:
    """Send rendered audio as audio_begin + N binary chunks + audio_end."""
    chunks = max(1, (len(pcm) + chunk_size - 1) // chunk_size)
    send_control(
        sock,
        {
            "type": "audio_begin",
            "audio_id": audio_id,
            "size": len(pcm),
            "chunks": chunks,
            "sha256": hashlib.sha256(pcm).hexdigest(),
            "format": audio_format,
            "sample_rate": sample_rate,
            "channels": channels,
        },
    )
    for seq in range(chunks):
        start = seq * chunk_size
        chunk = pcm[start : start + chunk_size]
        framing.send_frame(sock, framing.KIND_BINARY, _SEQ.pack(seq) + chunk)
    send_control(sock, {"type": "audio_end", "audio_id": audio_id})


@dataclass
class ReceivedAudio:
    audio_id: str
    audio_format: str
    sample_rate: int
    channels: int
    pcm: bytes


class AudioReassembler:
    """Receiver for one in-flight audio transfer (see ImageReassembler)."""

    def __init__(self, begin: dict):
        self.audio_id: str = begin["audio_id"]
        self.audio_format: str = begin.get("format", "pcm_s16le")
        self.sample_rate: int = begin["sample_rate"]
        self.channels: int = begin["channels"]
        self.expected_size: int = begin["size"]
        self.expected_chunks: int = begin["chunks"]
        self.expected_sha256: str = begin["sha256"]
        self._chunks: dict[int, bytes] = {}

    def add_chunk(self, payload: bytes) -> None:
        if len(payload) < _SEQ.size:
            raise framing.ProtocolError("binary chunk shorter than its header")
        (seq,) = _SEQ.unpack(payload[: _SEQ.size])
        self._chunks[seq] = payload[_SEQ.size :]

    def finish(self) -> ReceivedAudio:
        if len(self._chunks) != self.expected_chunks:
            raise framing.ProtocolError(
                f"chunk count mismatch: got {len(self._chunks)}, "
                f"expected {self.expected_chunks}"
            )
        pcm = b"".join(self._chunks[i] for i in range(self.expected_chunks))
        if len(pcm) != self.expected_size:
            raise framing.ProtocolError(
                f"size mismatch: got {len(pcm)}, expected {self.expected_size}"
            )
        if hashlib.sha256(pcm).hexdigest() != self.expected_sha256:
            raise framing.ProtocolError("sha256 mismatch — audio corrupted in transit")
        return ReceivedAudio(
            self.audio_id, self.audio_format, self.sample_rate, self.channels, pcm
        )


@dataclass
class ReceivedImage:
    image_id: str
    media_type: str
    data: bytes


class ImageReassembler:
    """Stateful receiver for one in-flight image transfer.

    Usage: feed it the image_begin control, then each binary frame's payload,
    then the image_end control. `finish()` verifies size + SHA-256 and returns
    the bytes.
    """

    def __init__(self, begin: dict):
        self.image_id: str = begin["image_id"]
        self.media_type: str = begin.get("media_type", "image/png")
        self.expected_size: int = begin["size"]
        self.expected_chunks: int = begin["chunks"]
        self.expected_sha256: str = begin["sha256"]
        self._chunks: dict[int, bytes] = {}

    def add_chunk(self, payload: bytes) -> None:
        if len(payload) < _SEQ.size:
            raise framing.ProtocolError("binary chunk shorter than its header")
        (seq,) = _SEQ.unpack(payload[: _SEQ.size])
        self._chunks[seq] = payload[_SEQ.size :]

    def finish(self) -> ReceivedImage:
        if len(self._chunks) != self.expected_chunks:
            raise framing.ProtocolError(
                f"chunk count mismatch: got {len(self._chunks)}, "
                f"expected {self.expected_chunks}"
            )
        data = b"".join(self._chunks[i] for i in range(self.expected_chunks))
        if len(data) != self.expected_size:
            raise framing.ProtocolError(
                f"size mismatch: got {len(data)}, expected {self.expected_size}"
            )
        actual_sha = hashlib.sha256(data).hexdigest()
        if actual_sha != self.expected_sha256:
            raise framing.ProtocolError("sha256 mismatch — image corrupted in transit")
        return ReceivedImage(self.image_id, self.media_type, data)
