"""Vendor-neutral AI provider layer.

CLOUD.md is explicit: "The system must never depend on a single AI vendor."
So every model call goes through the AIProvider interface, and the concrete
provider is chosen at runtime. Swapping OpenAI/Gemini/local-model in later means
adding one file here, not touching the pipeline.
"""

from __future__ import annotations

import os

from .provider import AIProvider
from .stub_provider import StubProvider


def get_provider(name: str | None = None) -> AIProvider:
    """Select an AI provider.

    Resolution order:
      1. explicit `name` argument, or the SUDONIT_AI_PROVIDER env var
      2. "anthropic" if an API key is present and the SDK is importable
      3. the deterministic stub (always works, no key, no network)

    This is what lets the whole loop run today on the stub and flip to real
    Claude the instant ANTHROPIC_API_KEY is set — with no code change.
    """
    name = name or os.environ.get("SUDONIT_AI_PROVIDER")

    if name == "stub":
        return StubProvider()

    if name == "anthropic" or (name is None and os.environ.get("ANTHROPIC_API_KEY")):
        try:
            from .anthropic_provider import AnthropicProvider

            return AnthropicProvider()
        except Exception as exc:  # SDK missing, key missing, etc.
            if name == "anthropic":
                raise  # caller explicitly asked for it — surface the failure
            # Auto-selection: degrade gracefully to the stub.
            print(f"[ai] anthropic unavailable ({exc}); using stub provider")

    return StubProvider()


__all__ = ["AIProvider", "StubProvider", "get_provider"]
