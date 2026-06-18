"""Structural image validation for incoming captures.

The reassembler (protocol/messages.py) proves the bytes arrived *intact*
(SHA-256), but not that they are a *decodable image*. That gap matters the day a
real camera driver is brought up: a truncated frame, a zero-length capture, or a
sensor that emits raw bytes under the wrong media_type would otherwise sail
straight into the AI provider and only fail at the real model — on hardware, at
the worst possible moment.

This module is a cheap, dependency-free *structural* check: it parses just the
PNG/JPEG headers to confirm the markers are present and the dimensions are sane.
It is deliberately not a full decode — pixel-level validity is the real model's
job. Catching the common bring-up failures here, on the phone, with a clear
error is what de-risks the camera integration.

Mock/raw media types (e.g. the host camera's "image/x-mock-rgb") are passed
through unchecked: they are not encoded images and have no header to parse.
"""

from __future__ import annotations

from dataclasses import dataclass

_PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"

# JPEG Start-Of-Frame markers carry the image dimensions. C4 (DHT), C8 (JPG) and
# CC (DAC) share the 0xCn range but are not frame headers, so they are excluded.
_JPEG_SOF_MARKERS = frozenset(
    {0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7, 0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF}
)
# Markers that stand alone (no length field): SOI, EOI, RSTn, TEM.
_JPEG_STANDALONE = frozenset({0xD8, 0xD9, 0x01} | set(range(0xD0, 0xD8)))

# Reject absurd dimensions early — a sane sensor frame is well within this.
_MAX_DIMENSION = 10000


class InvalidImage(Exception):
    """Raised when bytes claiming to be an image are not a valid one."""


@dataclass(frozen=True)
class ImageInfo:
    format: str  # "png" | "jpeg" | the pass-through media_type for raw formats
    width: int
    height: int


def _png_dimensions(data: bytes) -> tuple[int, int]:
    # 8-byte signature, then the IHDR chunk: length(4) "IHDR"(4) width(4) height(4).
    if len(data) < 24 or data[:8] != _PNG_SIGNATURE:
        raise InvalidImage("not a PNG (bad signature)")
    if data[12:16] != b"IHDR":
        raise InvalidImage("PNG missing IHDR chunk")
    width = int.from_bytes(data[16:20], "big")
    height = int.from_bytes(data[20:24], "big")
    return width, height


def _jpeg_dimensions(data: bytes) -> tuple[int, int]:
    n = len(data)
    if n < 2 or data[0:2] != b"\xff\xd8":
        raise InvalidImage("not a JPEG (missing SOI marker)")
    i = 2
    while i + 1 < n:
        if data[i] != 0xFF:  # resync to the next marker
            i += 1
            continue
        marker = data[i + 1]
        i += 2
        if marker in _JPEG_STANDALONE:
            continue
        if i + 2 > n:
            break
        seg_len = int.from_bytes(data[i : i + 2], "big")
        if seg_len < 2:
            raise InvalidImage("JPEG segment with invalid length")
        if marker in _JPEG_SOF_MARKERS:
            # SOF payload: length(2) precision(1) height(2) width(2) ...
            if i + 7 > n:
                raise InvalidImage("JPEG frame header truncated")
            height = int.from_bytes(data[i + 3 : i + 5], "big")
            width = int.from_bytes(data[i + 5 : i + 7], "big")
            return width, height
        i += seg_len
    raise InvalidImage("JPEG has no Start-Of-Frame marker")


def validate_image(data: bytes, media_type: str) -> ImageInfo:
    """Structurally validate `data` for `media_type`. Raises InvalidImage on
    failure; returns ImageInfo (with parsed dimensions) on success.

    Only image/png and image/jpeg are parsed. Any other media type is treated as
    an opaque/raw format and passed through (width/height = 0) — there is no
    header to check, and the host mock uses such a type deliberately."""
    if not data:
        raise InvalidImage("empty image payload")

    if media_type == "image/png":
        fmt, (width, height) = "png", _png_dimensions(data)
    elif media_type == "image/jpeg":
        fmt, (width, height) = "jpeg", _jpeg_dimensions(data)
    else:
        return ImageInfo(format=media_type, width=0, height=0)

    if not (1 <= width <= _MAX_DIMENSION and 1 <= height <= _MAX_DIMENSION):
        raise InvalidImage(f"{fmt} dimensions out of range: {width}x{height}")
    return ImageInfo(format=fmt, width=width, height=height)
