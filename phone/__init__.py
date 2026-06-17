"""The "phone brain" — the heavy-compute side of Sudonit.

Per MASTER.md / ARCHITECTURE.md, the glasses capture and the phone computes.
This package is the phone side of the V1 loop: receive an image, send it to an
AI provider, get a description, turn it into audio. It is written in Python so
the entire loop can run and be tested before any hardware exists; the logic
(provider interface, pipeline stages) ports directly to the Android app.
"""
