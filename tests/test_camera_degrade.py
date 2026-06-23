"""Tests for the camera degradation simulator."""

from __future__ import annotations

import io
import json

import numpy as np
from PIL import Image

from tools.camera_degrade import PRESETS, SVGA, DegradeParams, degrade, degrade_file


def _source(width=1280, height=960) -> Image.Image:
    """A deterministic, recognisable test image (gradient + blocks)."""
    arr = np.zeros((height, width, 3), dtype=np.uint8)
    xx = np.linspace(0, 255, width, dtype=np.uint8)
    arr[:, :, 1] = xx[None, :]
    arr[: height // 3, :, 0] = 200
    arr[2 * height // 3 :, :, 2] = 200
    return Image.fromarray(arr)


def test_output_is_svga_jpeg_rgb():
    decoded, used = degrade(_source(), PRESETS["typical"])
    assert decoded.mode == "RGB"
    # Fits within the sensor frame, aspect preserved.
    assert decoded.size[0] <= SVGA[0] and decoded.size[1] <= SVGA[1]
    assert used["output_format"] == "image/jpeg"
    assert used["output_size_bytes"] > 0
    assert used["output_resolution"] == list(decoded.size)


def test_all_presets_produce_valid_decodable_jpeg():
    src = _source()
    for name, params in PRESETS.items():
        decoded, used = degrade(src, params)
        # Round-trips through a real JPEG encode; must decode back cleanly.
        assert decoded.size[0] > 0 and decoded.size[1] > 0, name
        assert 1 <= used["jpeg_quality"] <= 95, name


def test_params_are_recorded_for_reproducibility():
    decoded, used = degrade(_source(), PRESETS["low_light"])
    # Every knob that produced this image is captured in the sidecar dict.
    for key in ("blur_radius", "motion_len", "light_gain", "noise_sigma",
                "jpeg_quality", "seed"):
        assert key in used


def test_degradation_is_deterministic_with_seed():
    src = _source()
    params = DegradeParams(noise_sigma=12.0, motion_len=6, seed=42, jpeg_quality=25)
    a, _ = degrade(src, params)
    b, _ = degrade(src, params)
    assert np.array_equal(np.asarray(a), np.asarray(b))


def test_lower_jpeg_quality_compresses_harder():
    # Isolate the compression knob (no noise — noise is high-entropy and would
    # *inflate* the JPEG, which is itself realistic but not what we assert here).
    src = _source()
    _, hi = degrade(src, DegradeParams(jpeg_quality=80))
    _, lo = degrade(src, DegradeParams(jpeg_quality=10))
    assert lo["output_size_bytes"] < hi["output_size_bytes"]


def test_sensor_noise_inflates_jpeg_size():
    # Documented realism: a noisy frame compresses *worse* than a clean one at
    # the same quality — the worst_case preset is bigger than clean for this
    # reason, not a bug.
    src = _source()
    _, clean = degrade(src, DegradeParams(jpeg_quality=25, noise_sigma=0.0))
    _, noisy = degrade(src, DegradeParams(jpeg_quality=25, noise_sigma=22.0))
    assert noisy["output_size_bytes"] > clean["output_size_bytes"]


def test_degrade_file_writes_image_and_sidecar(tmp_path):
    src_path = tmp_path / "in.png"
    _source().save(src_path)
    out_path = tmp_path / "out.jpg"
    used = degrade_file(src_path, out_path, PRESETS["typical"])

    assert out_path.exists()
    # Sidecar JSON next to the image, fully describing the degradation.
    sidecar = out_path.with_suffix(".params.json")
    assert sidecar.exists()
    loaded = json.loads(sidecar.read_text())
    assert loaded["jpeg_quality"] == used["jpeg_quality"]
    # The written file is a real JPEG.
    Image.open(io.BytesIO(out_path.read_bytes())).verify()


def test_low_light_actually_darkens():
    src = _source()
    bright, _ = degrade(src, PRESETS["clean"])
    dark, _ = degrade(src, PRESETS["low_light"])
    assert np.asarray(dark).mean() < np.asarray(bright).mean()
