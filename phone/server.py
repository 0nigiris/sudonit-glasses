"""Phone-side server: the data-plane endpoint the glasses connect to.

Listens on TCP (the Wi-Fi data-plane stand-in), speaks the Sudonit protocol,
reassembles an incoming image, runs it through the Pipeline, and sends the
result back to the glasses as `ai_response` + `play_audio` control messages —
exactly the round trip the real device will make.

Run:  python -m phone.server            # listens on 127.0.0.1:8765
"""

from __future__ import annotations

import argparse
import socket

from protocol import framing, messages

from .pipeline import Pipeline


def handle_connection(conn: socket.socket, pipeline: Pipeline) -> None:
    """Serve one glasses connection until it closes."""
    reassembler: messages.ImageReassembler | None = None

    while True:
        try:
            kind, payload = framing.recv_frame(conn)
        except framing.ProtocolError:
            return  # peer closed (cleanly or mid-frame); end this connection

        if kind == framing.KIND_BINARY:
            if reassembler is None:
                framing.send_frame(
                    conn,
                    framing.KIND_JSON,
                    messages.encode_control(
                        {"type": "error", "code": 1002, "message": "chunk before image_begin"}
                    ),
                )
                continue
            reassembler.add_chunk(payload)
            continue

        msg = messages.decode_control(payload)
        mtype = msg["type"]

        if mtype == "ping":
            messages.send_control(conn, {"type": "pong"})

        elif mtype == "device_info":
            messages.send_control(
                conn,
                {"type": "device_info", "device": "Sudonit Phone Brain", "firmware": "sim"},
            )

        elif mtype == "image_begin":
            reassembler = messages.ImageReassembler(msg)

        elif mtype == "image_end":
            if reassembler is None:
                continue
            image = reassembler.finish()
            reassembler = None
            _process_and_reply(conn, pipeline, image)

        else:
            messages.send_control(
                conn,
                {"type": "error", "code": 1001, "message": f"unhandled type {mtype}"},
            )


def _process_and_reply(
    conn: socket.socket, pipeline: Pipeline, image: messages.ReceivedImage
) -> None:
    print(f"[server] received image '{image.image_id}' ({len(image.data)} bytes)")
    result = pipeline.run(
        image.data,
        media_type=image.media_type,
        image_id=image.image_id,
        play=True,
    )
    print(f"[server] {result.provider}/{result.model} in {result.latency_ms} ms: {result.text}")
    messages.send_control(conn, {"type": "ai_response", "text": result.text})
    messages.send_control(conn, {"type": "play_audio", "content": result.text})


def serve(host: str, port: int) -> None:
    pipeline = Pipeline()
    print(f"[server] AI provider: {pipeline.provider.name}")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((host, port))
        srv.listen(1)
        print(f"[server] listening on {host}:{port}")
        while True:
            conn, addr = srv.accept()
            print(f"[server] glasses connected from {addr}")
            with conn:
                handle_connection(conn, pipeline)
            print("[server] glasses disconnected")


def main() -> None:
    parser = argparse.ArgumentParser(description="Sudonit phone-brain server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    serve(args.host, args.port)


if __name__ == "__main__":
    main()
