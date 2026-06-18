"""End-to-end test of the real server over a socket, with real image bytes.

Unlike the unit tests, this drives the actual `handle_connection` loop across a
socket pair: it sends a real image through the chunked transfer, lets the server
reassemble it, run the pipeline, and reply — then verifies both the text answer
and the rendered audio come back over the wire and survive SHA-256. This is the
"prove the server path with a real image" check that de-risks the day a real
camera frame first arrives.

TTS is stubbed to emit a deterministic 16-bit WAV so the audio downlink fires
regardless of whether espeak is installed on the dev box.
"""

import socket
import threading
import wave
from pathlib import Path

from phone.audio import TextToSpeech
from phone.pipeline import Pipeline
from phone.ai import StubProvider
from phone.server import handle_connection
from protocol import framing, messages
from tools.make_sample_image import make_png


class FakeTTS(TextToSpeech):
    """Writes a real, tiny 16-bit PCM WAV deterministically (no espeak needed)."""

    def synthesize(self, text: str, out_path) -> Path:
        out_path = Path(out_path)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with wave.open(str(out_path), "wb") as wav:
            wav.setnchannels(1)
            wav.setsampwidth(2)  # 16-bit
            wav.setframerate(16000)
            wav.writeframes(b"\x10\x00" * 800)  # 800 frames of a constant sample
        return out_path

    def play(self, wav_path) -> bool:
        return False


def _serve_once(conn: socket.socket, pipeline: Pipeline) -> None:
    try:
        handle_connection(conn, pipeline)
    finally:
        conn.close()


def test_server_roundtrip_real_image(tmp_path):
    png = make_png(tmp_path / "sample.png").read_bytes()
    pipeline = Pipeline(
        provider=StubProvider(), tts=FakeTTS(), output_dir=tmp_path / "run"
    )

    server_end, client_end = socket.socketpair()
    server = threading.Thread(target=_serve_once, args=(server_end, pipeline))
    server.start()
    try:
        messages.send_image(client_end, "shot-1", png, media_type="image/png")

        ai_text = None
        audio = None
        reassembler = None
        # Expect: ai_response (J), audio_begin (J), chunks (B...), audio_end (J).
        while audio is None:
            kind, payload = framing.recv_frame(client_end)
            if kind == framing.KIND_BINARY:
                assert reassembler is not None, "chunk before audio_begin"
                reassembler.add_chunk(payload)
                continue
            msg = messages.decode_control(payload)
            if msg["type"] == "ai_response":
                ai_text = msg["text"]
            elif msg["type"] == "audio_begin":
                reassembler = messages.AudioReassembler(msg)
            elif msg["type"] == "audio_end":
                audio = reassembler.finish()
    finally:
        client_end.close()
        server.join(timeout=5)

    assert ai_text and "stub" in ai_text.lower()
    assert audio is not None
    assert audio.sample_rate == 16000 and audio.channels == 1
    assert audio.audio_format == "pcm_s16le"
    assert len(audio.pcm) == 1600  # 800 frames * 2 bytes, SHA-256 verified in finish()


def test_server_rejects_corrupt_image(tmp_path):
    """A non-decodable JPEG is rejected by the pipeline, but the server still
    replies gracefully (a speakable error + its audio), never crashing."""
    pipeline = Pipeline(
        provider=StubProvider(), tts=FakeTTS(), output_dir=tmp_path / "run"
    )

    server_end, client_end = socket.socketpair()
    server = threading.Thread(target=_serve_once, args=(server_end, pipeline))
    server.start()
    try:
        # Valid SHA (intact in transit) but not a decodable JPEG.
        messages.send_image(client_end, "bad", b"\xff\xd8 garbage", media_type="image/jpeg")

        ai_text = None
        while ai_text is None:
            kind, payload = framing.recv_frame(client_end)
            if kind == framing.KIND_JSON:
                msg = messages.decode_control(payload)
                if msg["type"] == "ai_response":
                    ai_text = msg["text"]
    finally:
        client_end.close()
        server.join(timeout=5)

    assert "couldn't read" in ai_text.lower()
