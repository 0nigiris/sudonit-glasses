"""Sudonit communication protocol — the shared contract between glasses and phone.

Both the ESP32 firmware and the Android companion must implement this protocol
identically. The reference implementation here (Python) is what the simulator
and the phone-brain prototype use, and it doubles as executable documentation
of PROTOCOL.md + TRANSPORT.md.
"""

PROTOCOL_VERSION = "1.0"
