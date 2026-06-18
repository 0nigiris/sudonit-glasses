"""Audio output stage (text-to-speech)."""

from .tts import Pcm, TextToSpeech, pcm_from_wav

__all__ = ["TextToSpeech", "Pcm", "pcm_from_wav"]
