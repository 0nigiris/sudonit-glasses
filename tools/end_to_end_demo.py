"""END_TO_END_DEMO — the closest thing to the finished product without hardware.

Drives a real image through the *actual project code paths* and produces the WAV
the glasses' speaker would play:

    image bytes
      -> protocol framing + chunked image_begin/chunks/image_end  (protocol/)
      -> phone.server.handle_connection  (the real server loop)
      -> Pipeline: image validation -> AI provider -> text-to-speech  (phone/)
      -> audio_begin/chunks/audio_end downlink, SHA-verified  (protocol/)
      -> 16-bit PCM reassembled exactly as the firmware would receive it
      -> written out as a .wav you can listen to

Nothing here is a mock of the protocol or the server: it connects a real socket
pair and runs `phone.server.handle_connection` on one end, exactly as the ESP32
will talk to the phone over TCP. The only thing missing is the silicon.

With ANTHROPIC_API_KEY set, the description is from real Claude. Without it, the
deterministic stub answers — the audio path still runs end to end.

Usage:
    python tools/end_to_end_demo.py                      # uses a generated sample
    python tools/end_to_end_demo.py photo.jpg --out run/demo.wav --play
    python tools/end_to_end_demo.py photo.jpg --degrade typical   # camera-quality input
"""

from __future__ import annotations

import argparse
import socket
import sys
import threading
import time
import wave
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from phone.pipeline import Pipeline  # noqa: E402
from phone.server import handle_connection  # noqa: E402
from protocol import framing, messages  # noqa: E402

MEDIA_TYPES = {".png": "image/png", ".jpg": "image/jpeg", ".jpeg": "image/jpeg"}


def _media_type(path: Path) -> str:
    return MEDIA_TYPES.get(path.suffix.lower(), "image/jpeg")


def _write_wav(path: Path, audio: messages.ReceivedAudio) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(audio.channels)
        w.setsampwidth(2)  # 16-bit PCM — the firmware's fixed format
        w.setframerate(audio.sample_rate)
        w.writeframes(audio.pcm)


def run_demo(image_bytes: bytes, media_type: str, image_id: str = "demo"):
    """Run one full turn over a real socket pair. Returns (text, ReceivedAudio|None,
    latency_ms). Exercises framing, the server loop, the pipeline, and the audio
    downlink — the real code, end to end."""
    client, server = socket.socketpair()
    pipeline = Pipeline()
    print(f"[demo] provider: {pipeline.provider.name}")

    # The server side runs in a thread, just like the phone handling a connection.
    server.settimeout(60)
    t = threading.Thread(target=handle_connection, args=(server, pipeline), daemon=True)
    t.start()

    started = time.monotonic()
    # Uplink: announce + stream the image exactly as the ESP32 will.
    messages.send_image(client, image_id, image_bytes, media_type=media_type)

    text: str | None = None
    audio: messages.ReceivedAudio | None = None
    reassembler: messages.AudioReassembler | None = None
    client.settimeout(60)

    while True:
        try:
            kind, payload = framing.recv_frame(client)
        except (socket.timeout, framing.ProtocolError, OSError):
            break  # downlink finished (or audio was skipped: no TTS engine)
        if kind == framing.KIND_JSON:
            msg = messages.decode_control(payload)
            mtype = msg["type"]
            if mtype == "ai_response":
                text = msg["text"]
            elif mtype == "audio_begin":
                reassembler = messages.AudioReassembler(msg)
            elif mtype == "audio_end" and reassembler is not None:
                audio = reassembler.finish()
                break
            elif mtype == "error":
                text = f"[server error] {msg.get('message')}"
                break
        elif kind == framing.KIND_BINARY and reassembler is not None:
            reassembler.add_chunk(payload)

    latency_ms = int((time.monotonic() - started) * 1000)
    client.close()
    server.close()
    return text, audio, latency_ms


def _load_image(args) -> tuple[bytes, str, str]:
    if args.input:
        in_path = Path(args.input)
    else:
        # No input given: synthesize the project's sample PNG (stdlib only).
        from tools.make_sample_image import make_png

        in_path = Path("run/demo_input.png")
        make_png(in_path)
        print(f"[demo] no input given; generated sample {in_path}")

    if args.degrade:
        # Run the image through the camera-quality simulator first, so the demo
        # hears what the *real sensor* would feed the AI, not a pristine photo.
        from PIL import Image

        from tools.camera_degrade import PRESETS, degrade

        decoded, used = degrade(Image.open(in_path), PRESETS[args.degrade])
        out = Path("run") / f"{in_path.stem}.{args.degrade}.jpg"
        out.parent.mkdir(parents=True, exist_ok=True)
        decoded.save(out, format="JPEG", quality=used["jpeg_quality"])
        print(f"[demo] degraded input -> {out} ({used['output_size_bytes']} B)")
        return out.read_bytes(), "image/jpeg", out.stem

    return in_path.read_bytes(), _media_type(in_path), in_path.stem


def main() -> None:
    p = argparse.ArgumentParser(description="Full image->protocol->AI->TTS->wav demo.")
    p.add_argument("input", nargs="?", help="image file (default: generate a sample)")
    p.add_argument("--out", type=Path, default=Path("run/demo.wav"))
    p.add_argument("--degrade", choices=["clean", "good_light", "motion_blur",
                                          "typical", "low_light", "worst_case"],
                   help="first degrade the input to camera quality")
    p.add_argument("--play", action="store_true", help="play the wav if a player exists")
    args = p.parse_args()

    image_bytes, media_type, image_id = _load_image(args)
    print(f"[demo] sending {len(image_bytes)} B {media_type} as '{image_id}'")

    text, audio, latency_ms = run_demo(image_bytes, media_type, image_id)

    print("\n=== WHAT THE GLASSES WOULD SAY ===")
    print(f'  "{text}"')
    print(f"\nround-trip latency (in-process): {latency_ms} ms")
    if audio is not None:
        _write_wav(args.out, audio)
        secs = len(audio.pcm) / (2 * audio.channels * audio.sample_rate)
        print(f"audio: {len(audio.pcm)} B PCM, {audio.sample_rate} Hz x{audio.channels}, "
              f"{secs:.1f}s -> {args.out}")
        if args.play:
            from phone.audio import TextToSpeech

            played = TextToSpeech().play(args.out)
            print(f"playback: {'ok' if played else 'no audio player available'}")
    else:
        print("audio: none produced (no TTS engine on this machine; text only)")


if __name__ == "__main__":
    main()
