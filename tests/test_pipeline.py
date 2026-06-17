"""Pipeline tests against the deterministic stub provider (no key, no network)."""

from pathlib import Path

from phone.ai import StubProvider, get_provider
from phone.audio import TextToSpeech
from phone.pipeline import Pipeline
from tools.make_sample_image import make_png


def test_stub_provider_is_deterministic():
    provider = StubProvider()
    data = b"hello world bytes"
    a = provider.analyze_image(data)
    b = provider.analyze_image(data)
    assert a.text == b.text
    assert a.provider == "stub"


def test_pipeline_end_to_end_with_stub(tmp_path):
    image = make_png(tmp_path / "sample.png")
    pipeline = Pipeline(
        provider=StubProvider(),
        tts=TextToSpeech(),
        output_dir=tmp_path / "run",
    )
    result = pipeline.run(
        image.read_bytes(),
        media_type="image/png",
        image_id="sample",
        play=False,  # don't hit the audio device in tests
    )
    assert result.text  # something was "spoken"
    assert result.provider == "stub"
    assert Path(result.audio_path).exists()  # a .wav (or .txt fallback) was produced
    assert result.latency_ms >= 0


def test_get_provider_defaults_to_stub_without_key(monkeypatch):
    monkeypatch.delenv("ANTHROPIC_API_KEY", raising=False)
    monkeypatch.delenv("SUDONIT_AI_PROVIDER", raising=False)
    assert get_provider().name == "stub"
