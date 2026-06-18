"""Glasses simulator: stands in for the ESP32 until the hardware arrives.

It does what the firmware will do for the V1 loop:
  1. connect to the phone (TCP here / Wi-Fi later)
  2. ping it
  3. "capture" an image (read a file instead of an OV5640 frame)
  4. stream it over the protocol
  5. receive the AI response + play_audio back

When the OV5640 is wired up, only step 3 changes — everything below it is
already proven. That's the whole point of building this first.

    python -m simulator.glasses_sim [image_path]
"""

from __future__ import annotations

import argparse
import socket
import sys
from pathlib import Path

from protocol import framing, messages
from tools.make_sample_image import make_png

DEFAULT_IMAGE = Path("simulator/sample/sample.png")


def _load_image(path: Path) -> bytes:
    if not path.exists():
        print(f"[glasses] no image at {path}; generating a sample")
        make_png(path)
    return path.read_bytes()


def capture_and_send(host: str, port: int, image_path: Path) -> str:
    data = _load_image(image_path)
    with socket.create_connection((host, port), timeout=120) as sock:
        # 1. liveness check
        messages.send_control(sock, {"type": "ping"})
        kind, payload = framing.recv_frame(sock)
        assert messages.decode_control(payload)["type"] == "pong"
        print("[glasses] link up (pong received)")

        # 2. capture -> stream
        print(f"[glasses] sending capture '{image_path.name}' ({len(data)} bytes)")
        messages.send_image(sock, image_id=image_path.stem, data=data, media_type="image/png")

        # 3. wait for the AI response + the audio downlink coming back. This is
        #    the exact sequence the firmware handles: ai_response (text), then
        #    audio_begin + binary PCM chunks + audio_end. The device reassembles
        #    and plays the PCM; here we just report it.
        answer = ""
        audio_rx: messages.AudioReassembler | None = None
        while True:
            kind, payload = framing.recv_frame(sock)
            if kind == framing.KIND_BINARY:
                if audio_rx is not None:
                    audio_rx.add_chunk(payload)
                continue
            msg = messages.decode_control(payload)
            mtype = msg["type"]
            if mtype == "ai_response":
                answer = msg["text"]
                print(f"[glasses] AI response: {answer}")
            elif mtype == "audio_begin":
                audio_rx = messages.AudioReassembler(msg)
            elif mtype == "audio_end":
                audio = audio_rx.finish()  # verifies size + SHA-256
                print(
                    f"[glasses] (playing on speaker) {len(audio.pcm)} bytes PCM "
                    f"@ {audio.sample_rate} Hz, {audio.channels} ch"
                )
                break
            elif mtype == "play_audio":  # legacy text marker (no audio downlink)
                print(f"[glasses] (turn complete) {msg.get('content', '')}")
                break
            elif mtype == "error":
                print(f"[glasses] error {msg.get('code')}: {msg.get('message')}")
                break
        return answer


def main() -> int:
    parser = argparse.ArgumentParser(description="Sudonit glasses simulator")
    parser.add_argument("image", nargs="?", default=str(DEFAULT_IMAGE))
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    try:
        capture_and_send(args.host, args.port, Path(args.image))
    except (ConnectionRefusedError, OSError) as exc:
        print(f"[glasses] could not reach phone at {args.host}:{args.port} ({exc})")
        print("[glasses] start the phone first:  python -m phone.server")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
