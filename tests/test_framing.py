"""Protocol-layer tests: framing round-trips and chunked image transfer.

Uses a socketpair so we exercise the real send/recv path (including TCP's
partial-read behaviour) without opening a listening port.
"""

import socket

import pytest

from protocol import framing, messages


def test_json_frame_roundtrip():
    a, b = socket.socketpair()
    try:
        messages.send_control(a, {"type": "ping"})
        kind, payload = framing.recv_frame(b)
        assert kind == framing.KIND_JSON
        assert messages.decode_control(payload) == {"type": "ping"}
    finally:
        a.close()
        b.close()


def test_unknown_control_type_rejected():
    with pytest.raises(framing.ProtocolError):
        messages.encode_control({"type": "definitely_not_real"})


def test_image_transfer_reassembles_and_verifies():
    # An image larger than one chunk, to exercise multi-chunk reassembly.
    data = bytes(range(256)) * 50  # 12800 bytes -> 4 chunks at 4096
    a, b = socket.socketpair()
    try:
        messages.send_image(a, "img1", data, media_type="image/png", chunk_size=4096)

        # Receiver side: drive the reassembler from the frame stream.
        reasm = None
        result = None
        while result is None:
            kind, payload = framing.recv_frame(b)
            if kind == framing.KIND_JSON:
                msg = messages.decode_control(payload)
                if msg["type"] == "image_begin":
                    reasm = messages.ImageReassembler(msg)
                    assert reasm.expected_chunks == 4
                elif msg["type"] == "image_end":
                    result = reasm.finish()
            else:
                reasm.add_chunk(payload)

        assert result.data == data
        assert result.media_type == "image/png"
    finally:
        a.close()
        b.close()


def test_corrupted_chunk_fails_checksum():
    begin = {
        "type": "image_begin",
        "image_id": "x",
        "size": 10,
        "chunks": 1,
        "sha256": "0" * 64,  # wrong on purpose
        "media_type": "image/png",
    }
    reasm = messages.ImageReassembler(begin)
    import struct

    reasm.add_chunk(struct.pack(">I", 0) + b"0123456789")
    with pytest.raises(framing.ProtocolError):
        reasm.finish()


# --- Malformed-peer hardening -------------------------------------------------
#
# A device on real Wi-Fi can send a truncated, mistyped, or non-UTF-8 control
# frame. The server only treats framing.ProtocolError as "end this connection";
# any other exception escaping the reassembler crashes the whole server. These
# tests pin every malformed-input path to ProtocolError so that boundary holds.


@pytest.mark.parametrize("missing", ["image_id", "size", "chunks", "sha256"])
def test_image_begin_missing_field_raises_protocol_error(missing):
    begin = {
        "type": "image_begin",
        "image_id": "x",
        "size": 10,
        "chunks": 1,
        "sha256": "0" * 64,
        "media_type": "image/png",
    }
    del begin[missing]
    with pytest.raises(framing.ProtocolError):
        messages.ImageReassembler(begin)


@pytest.mark.parametrize("missing", ["audio_id", "sample_rate", "channels", "size", "chunks", "sha256"])
def test_audio_begin_missing_field_raises_protocol_error(missing):
    begin = {
        "type": "audio_begin",
        "audio_id": "a",
        "sample_rate": 16000,
        "channels": 1,
        "size": 10,
        "chunks": 1,
        "sha256": "0" * 64,
    }
    del begin[missing]
    with pytest.raises(framing.ProtocolError):
        messages.AudioReassembler(begin)


def test_image_begin_non_integer_size_raises_protocol_error():
    begin = {
        "type": "image_begin",
        "image_id": "x",
        "size": "not-a-number",
        "chunks": 1,
        "sha256": "0" * 64,
    }
    with pytest.raises(framing.ProtocolError):
        messages.ImageReassembler(begin)


def test_image_finish_wrong_seq_raises_protocol_error():
    # Right chunk *count* but the wrong seq number: range(expected_chunks) would
    # KeyError without the guard. Must surface as ProtocolError, not crash.
    import struct

    begin = {
        "type": "image_begin",
        "image_id": "x",
        "size": 10,
        "chunks": 1,
        "sha256": "0" * 64,
    }
    reasm = messages.ImageReassembler(begin)
    reasm.add_chunk(struct.pack(">I", 7) + b"0123456789")  # seq 7, not 0
    with pytest.raises(framing.ProtocolError):
        reasm.finish()


def test_audio_finish_wrong_seq_raises_protocol_error():
    import struct

    begin = {
        "type": "audio_begin",
        "audio_id": "a",
        "sample_rate": 16000,
        "channels": 1,
        "size": 10,
        "chunks": 1,
        "sha256": "0" * 64,
    }
    reasm = messages.AudioReassembler(begin)
    reasm.add_chunk(struct.pack(">I", 7) + b"0123456789")  # seq 7, not 0
    with pytest.raises(framing.ProtocolError):
        reasm.finish()


def test_decode_control_bad_json_raises_protocol_error():
    with pytest.raises(framing.ProtocolError):
        messages.decode_control(b"{not valid json")


def test_decode_control_non_utf8_raises_protocol_error():
    with pytest.raises(framing.ProtocolError):
        messages.decode_control(b"\xff\xfe\xfd")


def test_decode_control_missing_type_raises_protocol_error():
    with pytest.raises(framing.ProtocolError):
        messages.decode_control(b'{"no_type": 1}')


def test_decode_control_non_object_raises_protocol_error():
    with pytest.raises(framing.ProtocolError):
        messages.decode_control(b"[1, 2, 3]")
