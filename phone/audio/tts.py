"""Text-to-speech: turn the AI's answer into audio.

In V1 the *phone* renders speech and (later) streams it to the glasses' speaker
(ARCHITECTURE.md audio pipeline). On this dev machine we render with espeak-ng
to a WAV file and optionally play it with aplay. Both are wrapped so a headless
or audio-less environment degrades gracefully instead of crashing the loop —
the WAV is still produced, which is what tests assert on.

This is intentionally behind a small class so the real Android implementation
(Android TextToSpeech / a streamed Opus codec to the ESP32) is a drop-in swap.
"""

from __future__ import annotations

import shutil
import subprocess
import wave
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Pcm:
    """Raw 16-bit little-endian PCM plus the metadata needed to play it. This is
    exactly what the firmware's sd_audio_play_pcm() / the audio downlink need."""

    data: bytes
    sample_rate: int
    channels: int


def pcm_from_wav(wav_path: str | Path) -> Pcm:
    """Extract raw 16-bit PCM samples from a WAV file (stdlib only).

    Raises ValueError if the file is not 16-bit PCM — the glasses speaker path is
    fixed at 16-bit, so anything else is a producer bug worth surfacing early."""
    with wave.open(str(wav_path), "rb") as wav:
        if wav.getsampwidth() != 2:
            raise ValueError(
                f"expected 16-bit PCM, got {wav.getsampwidth() * 8}-bit"
            )
        frames = wav.readframes(wav.getnframes())
        return Pcm(frames, wav.getframerate(), wav.getnchannels())


class TextToSpeech:
    def __init__(self, voice: str = "en", speed_wpm: int = 165):
        self.voice = voice
        self.speed_wpm = speed_wpm
        self._espeak = shutil.which("espeak-ng") or shutil.which("espeak")
        self._player = shutil.which("aplay") or shutil.which("paplay")

    @property
    def available(self) -> bool:
        return self._espeak is not None

    def synthesize(self, text: str, out_path: str | Path) -> Path:
        """Render `text` to a WAV file. Returns the path.

        If no TTS engine is installed, writes the text alongside as a .txt so
        the pipeline still produces an artifact and the failure is visible.
        """
        out_path = Path(out_path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        if not self._espeak:
            fallback = out_path.with_suffix(".txt")
            fallback.write_text(text, encoding="utf-8")
            return fallback
        subprocess.run(
            [
                self._espeak,
                "-v",
                self.voice,
                "-s",
                str(self.speed_wpm),
                "-w",
                str(out_path),
                text,
            ],
            check=True,
            capture_output=True,
        )
        return out_path

    def play(self, wav_path: str | Path) -> bool:
        """Play a WAV file if a player is available. Returns True if it played."""
        wav_path = Path(wav_path)
        if not self._player or wav_path.suffix != ".wav" or not wav_path.exists():
            return False
        try:
            subprocess.run(
                [self._player, str(wav_path)], check=True, capture_output=True
            )
            return True
        except subprocess.CalledProcessError:
            return False
