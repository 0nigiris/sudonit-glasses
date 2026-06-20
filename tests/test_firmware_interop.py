"""Automated cross-stack interop test: the real host-built firmware loop talks to
the real Python phone server over a real TCP socket, and we *assert* the turn.

Why this exists
---------------
`firmware/src/app/main_interop.c` (the `device_interop` binary) already runs the
real `sd_device_run_uplink` — capture (mock camera) -> chunked protocol over TCP
-> phone server -> AI -> spoken-audio downlink -> played back through the audio
HAL. But until now it was only ever driven by hand (`run_interop.sh`) and nothing
*checked* the result: a regression in framing.c, messages.c, the server loop, or
device.c could break the entire demo path and every existing test would still
pass. This is the single most important integration path for the first hardware
demo, and it had zero automated coverage.

This test closes that gap. It builds the firmware host binary, stands up the real
server in-process with a deterministic TTS (so the audio downlink fires on any
machine, espeak or not — mirroring tests/test_server_e2e.py), runs the real C
binary against it, and asserts:

  * the process exits 0 (the full turn completed),
  * a non-empty AI text answer came back over the C protocol,
  * PCM audio frames were actually played back through the firmware audio HAL.

When hardware arrives, the esp32 build links the same device.c against the real
camera/audio/Wi-Fi backends; this test is the host-side contract it must keep
satisfying. It is a permanent regression guard for the demo loop, not a one-off.
"""

from __future__ import annotations

import re
import shutil
import socket
import subprocess
import threading
import wave
from pathlib import Path

import pytest

from phone.ai import StubProvider
from phone.audio import TextToSpeech
from phone.pipeline import Pipeline
from phone.server import handle_connection

REPO_ROOT = Path(__file__).resolve().parent.parent
FIRMWARE_DIR = REPO_ROOT / "firmware"
BUILD_DIR = FIRMWARE_DIR / "build"

# FakeTTS writes exactly this many 16-bit mono frames, so the firmware must play
# back exactly this many — a strong, deterministic end-to-end audio assertion.
FAKE_TTS_FRAMES = 800


class FakeTTS(TextToSpeech):
    """Deterministic 16-bit PCM WAV, no espeak dependency (see test_server_e2e)."""

    def synthesize(self, text: str, out_path) -> Path:
        out_path = Path(out_path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with wave.open(str(out_path), "wb") as wav:
            wav.setnchannels(1)
            wav.setsampwidth(2)  # 16-bit
            wav.setframerate(16000)
            wav.writeframes(b"\x10\x00" * FAKE_TTS_FRAMES)
        return out_path

    def play(self, wav_path) -> bool:
        return False


@pytest.fixture(scope="module")
def interop_binary() -> Path:
    """Build (incrementally) the firmware host `device_interop` binary.

    Skips cleanly where there is no C toolchain (so `pytest` still passes on a
    machine that can't build the firmware), and rebuilds every run so source
    changes to device.c / main_interop.c are always exercised.
    """
    cmake = shutil.which("cmake")
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if not cmake or not compiler:
        pytest.skip("no cmake / C compiler available; cannot build the firmware host loop")

    if not (BUILD_DIR / "CMakeCache.txt").exists():
        cfg = subprocess.run(
            [cmake, "-S", str(FIRMWARE_DIR), "-B", str(BUILD_DIR)],
            capture_output=True, text=True,
        )
        if cfg.returncode != 0:
            pytest.fail("cmake configure failed:\n" + cfg.stdout + cfg.stderr)

    build = subprocess.run(
        [cmake, "--build", str(BUILD_DIR), "--target", "device_interop"],
        capture_output=True, text=True,
    )
    if build.returncode != 0:
        pytest.fail("firmware host build failed:\n" + build.stdout + build.stderr)

    binary = BUILD_DIR / "device_interop"
    assert binary.exists(), "device_interop binary missing after a successful build"
    return binary


def _serve_one_connection(srv: socket.socket, pipeline: Pipeline) -> None:
    """Accept exactly one glasses connection and serve the turn, then stop."""
    try:
        conn, _ = srv.accept()
    except OSError:
        return  # listener closed (e.g. the binary never connected) — let the test fail loudly
    with conn:
        handle_connection(conn, pipeline)


def test_firmware_loop_completes_a_real_turn(interop_binary, tmp_path):
    pipeline = Pipeline(
        provider=StubProvider(), tts=FakeTTS(), output_dir=tmp_path / "run"
    )

    # Real TCP listener on an ephemeral port — the C binary connects to this.
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", 0))
    port = srv.getsockname()[1]
    srv.listen(1)
    srv.settimeout(60)

    server = threading.Thread(target=_serve_one_connection, args=(srv, pipeline), daemon=True)
    server.start()

    try:
        proc = subprocess.run(
            [str(interop_binary), "127.0.0.1", str(port)],
            capture_output=True, text=True, timeout=60,
        )
    finally:
        server.join(timeout=5)
        srv.close()

    # The full turn must have completed cleanly.
    assert proc.returncode == 0, (
        f"device_interop exited {proc.returncode}\n"
        f"--- stdout ---\n{proc.stdout}\n--- stderr ---\n{proc.stderr}"
    )

    # A real AI text answer came back over the C protocol and into the app layer.
    m = re.search(r"PHONE RESPONSE: (.+)", proc.stdout)
    assert m, f"no AI response in firmware output:\n{proc.stdout}"
    response = m.group(1).strip()
    assert response, "AI response was empty"
    assert "stub" in response.lower(), f"unexpected AI response: {response!r}"

    # The audio downlink reached the firmware and was played back through the HAL.
    frames = re.search(r"audio_frames=(\d+)", proc.stdout)
    assert frames, f"firmware did not report audio frames:\n{proc.stdout}"
    played = int(frames.group(1))
    assert played > 0, "no audio frames were played — the audio downlink did not complete"
    assert played == FAKE_TTS_FRAMES, (
        f"played {played} frames, expected {FAKE_TTS_FRAMES} "
        "(PCM lost or miscounted across the protocol downlink)"
    )
