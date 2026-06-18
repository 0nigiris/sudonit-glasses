"""Tests for the structural image validator (phone/image.py)."""

import pytest

from phone.image import InvalidImage, validate_image
from tools.make_sample_image import make_png


def _tiny_jpeg(width: int, height: int) -> bytes:
    """A structurally valid baseline-JPEG header (SOI + SOF0 + EOI). Enough for
    the validator's marker/dimension parse; not a fully decodable image."""
    sof = (
        b"\xff\xc0"
        + (17).to_bytes(2, "big")  # segment length: 8 + 3 components * 3
        + b"\x08"  # 8-bit precision
        + height.to_bytes(2, "big")
        + width.to_bytes(2, "big")
        + b"\x03\x01\x22\x00\x02\x11\x01\x03\x11\x01"  # 3 component descriptors
    )
    return b"\xff\xd8" + sof + b"\xff\xd9"


def test_validates_real_png(tmp_path):
    png = make_png(tmp_path / "sample.png").read_bytes()
    info = validate_image(png, "image/png")
    assert info.format == "png"
    assert (info.width, info.height) == (320, 240)


def test_validates_jpeg_dimensions():
    info = validate_image(_tiny_jpeg(640, 480), "image/jpeg")
    assert info.format == "jpeg"
    assert (info.width, info.height) == (640, 480)


def test_rejects_empty_payload():
    with pytest.raises(InvalidImage):
        validate_image(b"", "image/jpeg")


def test_rejects_truncated_png(tmp_path):
    png = make_png(tmp_path / "sample.png").read_bytes()
    with pytest.raises(InvalidImage):
        validate_image(png[:10], "image/png")  # signature only, no IHDR


def test_rejects_garbage_jpeg():
    with pytest.raises(InvalidImage):
        validate_image(b"\xff\xd8 not really a jpeg", "image/jpeg")  # SOI, no SOF


def test_rejects_wrong_magic():
    with pytest.raises(InvalidImage):
        validate_image(b"\x89PNG\r\n\x1a\n" + b"\x00" * 16, "image/jpeg")


def test_passes_through_raw_mock_type():
    # The host mock camera emits raw bytes under a non-image media type; there is
    # no header to parse, so validation must not reject it.
    info = validate_image(b"\x01\x02\x03\x04", "image/x-mock-rgb")
    assert info.format == "image/x-mock-rgb"
    assert (info.width, info.height) == (0, 0)
