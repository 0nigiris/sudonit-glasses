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
from pathlib import Path


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
