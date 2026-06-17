"""Anthropic (Claude) vision provider.

Built against the official `anthropic` SDK per Anthropic's own integration
guidance. Defaults to claude-opus-4-8 (multimodal, the current most-capable
Opus). We deliberately do NOT enable extended/adaptive thinking here: image
description for a wearable is a single-shot, latency- and cost-sensitive call,
and the product priorities (reliability, battery, speed) favour a fast answer
over deep reasoning. Override the model with SUDONIT_AI_MODEL if needed.

The API key is read from the environment (ANTHROPIC_API_KEY) — never hardcoded
(CLAUDE.md: "Never hardcode secrets").
"""

from __future__ import annotations

import base64
import os

from .provider import AIProvider, AIResult, DEFAULT_PROMPT

# Configurable, but defaults to the current most-capable Opus model.
DEFAULT_MODEL = os.environ.get("SUDONIT_AI_MODEL", "claude-opus-4-8")

# Short cap: we want one or two spoken sentences, not a wall of text.
MAX_TOKENS = 300


class AnthropicProvider(AIProvider):
    name = "anthropic"

    def __init__(self, model: str = DEFAULT_MODEL):
        # Imported lazily so a machine without the SDK can still run the stub.
        import anthropic

        if not os.environ.get("ANTHROPIC_API_KEY"):
            raise RuntimeError("ANTHROPIC_API_KEY is not set")
        self.model = model
        self._client = anthropic.Anthropic()

    def analyze_image(
        self,
        image_bytes: bytes,
        media_type: str = "image/png",
        prompt: str = DEFAULT_PROMPT,
    ) -> AIResult:
        b64 = base64.standard_b64encode(image_bytes).decode("ascii")
        response = self._client.messages.create(
            model=self.model,
            max_tokens=MAX_TOKENS,
            messages=[
                {
                    "role": "user",
                    "content": [
                        {
                            "type": "image",
                            "source": {
                                "type": "base64",
                                "media_type": media_type,
                                "data": b64,
                            },
                        },
                        {"type": "text", "text": prompt},
                    ],
                }
            ],
        )
        # response.content is a list of typed blocks; collect the text ones.
        text = " ".join(
            block.text for block in response.content if block.type == "text"
        ).strip()
        if not text:
            # e.g. a safety refusal — surface something speakable rather than ""
            text = "I wasn't able to describe that image."
        return AIResult(text=text, provider=self.name, model=response.model)
