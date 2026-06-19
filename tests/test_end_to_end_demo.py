"""Tests for the end-to-end demo: image -> protocol -> AI -> TTS -> wav.

Runs over a real socket pair against the real server loop and pipeline, on the
deterministic stub provider. Asserts the loop completes and (when a TTS engine is
present) produces playable 16-bit PCM — exactly what the firmware would receive.
"""

from __future__ import annotations

import wave

import pytest

from tools.end_to_end_demo import _write_wav, run_demo
from tools.make_sample_image import make_png


@pytest.fixture(autouse=True)
def force_stub(monkeypatch):
    monkeypatch.setenv("SUDONIT_AI_PROVIDER", "stub")


def _png_bytes(tmp_path):
    p = make_png(tmp_path / "scene.png")
    return p.read_bytes()


def test_loop_returns_text_answer(tmp_path):
    text, audio, latency_ms = run_demo(_png_bytes(tmp_path), "image/png", "scene")
    # The stub always answers; the real product replaces this with Claude's text.
    assert text and "Stub analysis" in text
    assert isinstance(latency_ms, int) and latency_ms >= 0


def test_audio_downlink_is_16bit_pcm_when_tts_available(tmp_path):
    _text, audio, _ = run_demo(_png_bytes(tmp_path), "image/png", "scene")
    if audio is None:
        pytest.skip("no TTS engine on this machine; audio path not exercised")
    # The firmware's speaker path is fixed at 16-bit PCM.
    assert len(audio.pcm) > 0
    assert audio.sample_rate > 0
    assert audio.channels in (1, 2)
    # PCM length must be a whole number of 16-bit frames.
    assert len(audio.pcm) % (2 * audio.channels) == 0


def test_written_wav_is_valid(tmp_path):
    _text, audio, _ = run_demo(_png_bytes(tmp_path), "image/png", "scene")
    if audio is None:
        pytest.skip("no TTS engine on this machine")
    out = tmp_path / "demo.wav"
    _write_wav(out, audio)
    with wave.open(str(out), "rb") as w:
        assert w.getsampwidth() == 2  # 16-bit
        assert w.getframerate() == audio.sample_rate
        assert w.getnframes() > 0


def test_degraded_input_still_completes(tmp_path):
    # Tie tool 1 to tool 3: a camera-quality (JPEG) frame must traverse the loop.
    from PIL import Image

    from tools.camera_degrade import PRESETS, degrade

    src = Image.open(make_png(tmp_path / "scene.png"))
    decoded, used = degrade(src, PRESETS["typical"])
    out = tmp_path / "scene.typical.jpg"
    decoded.save(out, format="JPEG", quality=used["jpeg_quality"])

    text, _audio, _ = run_demo(out.read_bytes(), "image/jpeg", "scene_degraded")
    assert text and "Stub analysis" in text
