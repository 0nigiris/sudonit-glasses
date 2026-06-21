"""Generate a small sample PNG with zero third-party dependencies.

We can't assume Pillow is installed (and the firmware certainly won't have it),
so this writes a valid truecolor PNG by hand using only stdlib zlib. The image
is a blue->green gradient with a red square and a yellow bar — enough recognisable
content that a real vision model returns a sensible description, and enough to
prove the capture path with the stub.

    python tools/make_sample_image.py [out_path]
"""

from __future__ import annotations

import struct
import sys
import zlib
from pathlib import Path

WIDTH, HEIGHT = 320, 240


def _chunk(tag: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + tag
        + data
        + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    )


def _pixels() -> bytes:
    rows = bytearray()
    for y in range(HEIGHT):
        rows.append(0)  # PNG filter type 0 (none) for this scanline
        for x in range(WIDTH):
            # Base gradient: blue fades down, green grows right.
            r = 20
            g = int(255 * x / WIDTH)
            b = int(255 * (1 - y / HEIGHT))
            # A red square in the upper-left quadrant.
            if 40 <= x < 120 and 40 <= y < 120:
                r, g, b = 220, 30, 30
            # A yellow horizontal bar near the bottom.
            if 170 <= y < 200:
                r, g, b = 240, 220, 40
            rows.extend((r, g, b))
    return bytes(rows)


def make_png(out_path: str | Path) -> Path:
    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    ihdr = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0)  # 8-bit truecolor
    png = (
        b"\x89PNG\r\n\x1a\n"
        + _chunk(b"IHDR", ihdr)
        + _chunk(b"IDAT", zlib.compress(_pixels(), 9))
        + _chunk(b"IEND", b"")
    )
    out_path.write_bytes(png)
    return out_path


if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else "run/sample.png"
    path = make_png(target)
    print(f"wrote {path} ({path.stat().st_size} bytes)")
