# RUNNING.md — running the Sudonit prototype

This is the hardware-free vertical slice of the V1 loop:

```
capture image -> transport -> AI provider -> text -> audio
```

It runs entirely on a development machine. No ESP32, no phone, no camera
required — the glasses are simulated and the "phone brain" runs locally. When
the hardware arrives, only the capture source changes; everything below it is
already proven here.

---

## Requirements

- Linux, Python 3.10+
- `espeak-ng` and `aplay` for audio (already present on most desktop installs)
- Python packages:
  - `pytest` — to run the tests
  - `anthropic` — **only** needed for the real Claude provider; the deterministic
    stub provider needs nothing beyond the standard library

```bash
python3 -m pip install anthropic pytest
```

The loop runs with **zero** third-party packages if you stay on the stub
provider (no API key) — `anthropic` is optional.

---

## Run the tests

No API key, no network needed (tests use the deterministic stub):

```bash
python3 -m pytest -q
```

Expected: `7 passed`.

---

## Run the full demo

Two terminals.

**Terminal A — the phone brain (data-plane server):**

```bash
python3 -m phone.server
```

It prints the selected AI provider and listens on `127.0.0.1:8765`.

**Terminal B — the firmware host loop (capture + send):**

Build the host firmware once, then run the real device loop against the server:

```bash
cmake -S firmware -B firmware/build && cmake --build firmware/build
./firmware/build/device_interop 127.0.0.1 8765
```

This runs the actual firmware `device.c` (mock camera capture) over the real
protocol — the same code the ESP32 build links against the hardware backends.

What you'll see: the glasses ping the phone, stream the image over the chunked
protocol, and receive the AI response back. The phone writes the spoken answer
to `run/<image-id>.wav` and plays it.

---

## Use real Claude vision instead of the stub

Set an API key before starting the phone server — no code change required:

```bash
export ANTHROPIC_API_KEY=sk-ant-...
python3 -m phone.server
```

The provider is auto-selected at startup (`phone/ai/__init__.py`): Claude if a
key is present and the SDK is installed, otherwise the stub.

Optional overrides:

| Env var                | Default            | Meaning                          |
|------------------------|--------------------|----------------------------------|
| `ANTHROPIC_API_KEY`    | _(unset)_          | Enables the real Claude provider |
| `SUDONIT_AI_PROVIDER`  | auto               | Force `anthropic` or `stub`      |
| `SUDONIT_AI_MODEL`     | `claude-opus-4-8`  | Vision model id                  |

---

## Generate a sample image manually

```bash
python3 tools/make_sample_image.py [out_path]
```

Pure-stdlib PNG writer (no Pillow) — a gradient with a red square and a yellow
bar, enough for a real vision model to describe.

---

## Layout

| Path          | What it is                                                    |
|---------------|---------------------------------------------------------------|
| `protocol/`   | Shared wire contract: framing + chunked image transfer. Both firmware and the Android app will implement this. See `protocol/TRANSPORT.md`. |
| `phone/`      | The phone brain: AI provider interface, pipeline, TCP server. |
| `simulator/`  | Hardware-free glasses stand-in.                               |
| `tools/`      | Helper scripts (sample image generator).                      |
| `tests/`      | Protocol + pipeline tests.                                    |
| `run/`        | Generated audio output (gitignored).                          |
