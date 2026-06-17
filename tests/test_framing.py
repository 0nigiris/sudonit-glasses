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
