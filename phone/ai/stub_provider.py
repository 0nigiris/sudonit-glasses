"""Deterministic, offline AI provider.

This exists so the entire pipeline (and the tests, and the CI) can run with no
API key, no network, and no cost — and produce the *same* answer every time.
That determinism is what makes the loop testable. It is NOT a real model; it
just reports verifiable facts about the bytes it received, which is enough to
prove the transport + audio stages end-to-end.
"""

from __future__ import annotations

import hashlib

from .provider import AIProvider, AIResult, DEFAULT_PROMPT


class StubProvider(AIProvider):
    name = "stub"

    def analyze_image(
        self,
        image_bytes: bytes,
        media_type: str = "image/png",
        prompt: str = DEFAULT_PROMPT,
    ) -> AIResult:
        digest = hashlib.sha256(image_bytes).hexdigest()[:12]
        kb = len(image_bytes) / 1024
        text = (
            f"Stub analysis: I received a {kb:.1f} kilobyte {media_type} image "
            f"(fingerprint {digest}). Set ANTHROPIC_API_KEY for a real "
            f"description."
        )
        return AIResult(text=text, provider=self.name, model="stub-deterministic")
