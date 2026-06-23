"""The AIProvider interface — the contract every backend implements."""

from __future__ import annotations

import abc
from dataclasses import dataclass

# The default instruction sent with an image. Kept short and concrete: the
# glasses use case wants a quick, speakable answer, not an essay (MASTER.md
# priorities put reliability/battery/comfort above features; OPUS_BRIEF wants
# the device to "feel invisible").
DEFAULT_PROMPT = (
    "You are the assistant inside a pair of smart glasses. The user just "
    "captured this image and wants to know what they are looking at. Reply in "
    "one or two short, natural spoken sentences — no preamble, no markdown."
)


@dataclass
class AIResult:
    text: str
    provider: str
    model: str
    # Token accounting, when the backend reports it (the Anthropic API does;
    # the offline stub does not). Left as None for providers that can't measure
    # it, so cost/usage tooling can distinguish "zero" from "unknown".
    input_tokens: int | None = None
    output_tokens: int | None = None


class AIProvider(abc.ABC):
    """Analyze an image and return a short spoken-style description.

    Implementations must be safe to construct cheaply and must not require a
    network connection at import time (so the stub path stays dependency-free).
    """

    name: str = "abstract"

    @abc.abstractmethod
    def analyze_image(
        self,
        image_bytes: bytes,
        media_type: str = "image/png",
        prompt: str = DEFAULT_PROMPT,
    ) -> AIResult:
        """Return an AIResult describing the image."""
        raise NotImplementedError
