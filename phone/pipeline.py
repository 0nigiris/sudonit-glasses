"""The core V1 loop, orchestrated in one place and free of any transport.

    image bytes -> AI provider -> text -> text-to-speech -> audio file

Keeping this socket-free is deliberate: it's the part that carries the product
value, and it must be unit-testable without hardware or even a network. The
server (server.py) and the future Android app are just transports that feed
bytes into `Pipeline.run`.
"""

from __future__ import annotations

import time
from dataclasses import dataclass
from pathlib import Path

from .ai import AIProvider, get_provider
from .ai.provider import DEFAULT_PROMPT
from .audio import TextToSpeech


@dataclass
class PipelineResult:
    text: str
    provider: str
    model: str
    audio_path: Path
    audio_played: bool
    latency_ms: int


class Pipeline:
    def __init__(
        self,
        provider: AIProvider | None = None,
        tts: TextToSpeech | None = None,
        output_dir: str | Path = "run",
    ):
        self.provider = provider or get_provider()
        self.tts = tts or TextToSpeech()
        self.output_dir = Path(output_dir)

    def run(
        self,
        image_bytes: bytes,
        media_type: str = "image/png",
        image_id: str = "capture",
        prompt: str = DEFAULT_PROMPT,
        play: bool = True,
    ) -> PipelineResult:
        started = time.monotonic()

        result = self.provider.analyze_image(image_bytes, media_type, prompt)

        wav_path = self.output_dir / f"{image_id}.wav"
        audio_path = self.tts.synthesize(result.text, wav_path)
        audio_played = self.tts.play(audio_path) if play else False

        latency_ms = int((time.monotonic() - started) * 1000)
        return PipelineResult(
            text=result.text,
            provider=result.provider,
            model=result.model,
            audio_path=audio_path,
            audio_played=audio_played,
            latency_ms=latency_ms,
        )
