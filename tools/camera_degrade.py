"""CAMERA_DEGRADATION_SIMULATOR — push a normal photo toward what the real
OV5640 at SVGA/JPEG will actually deliver, so we can measure how much image
quality loss costs us in AI usefulness *before the camera exists*.

The firmware captures `FRAMESIZE_SVGA` (800x600) JPEG at `jpeg_quality = 12`
(esp32-camera scale 0..63, lower = better) — see firmware/src/hal/esp32/
camera_esp.c. A clip-on camera on glasses also realistically suffers: soft
optics / slight defocus, hand-and-head motion blur, and — the big one — low
light, which both darkens the frame and amplifies sensor noise.

This tool models those, each independently parameterised, and records the exact
parameters it used alongside the output, so a degraded image is always
reproducible and explainable.

Pipeline order (matches physical reality): resize to sensor resolution -> optical
blur -> motion blur -> low-light response (gain/gamma) -> sensor noise -> JPEG
compression. Noise before JPEG so the encoder sees (and partially smears) it,
exactly as on a real sensor.

Usage:
    # one image with a named preset
    python tools/camera_degrade.py in.jpg --preset typical --out out.jpg

    # a whole folder, all presets, into a grid of artifacts
    python tools/camera_degrade.py in.jpg --all-presets --outdir run/degraded

    # custom knobs (override any preset field)
    python tools/camera_degrade.py in.jpg --preset clean --blur 1.5 --noise 8

Presets (clean -> worst_case) bracket the plausible range; `typical` is the
honest default expectation for an indoor, handheld capture.

As a library:
    from tools.camera_degrade import degrade, PRESETS
    out_img, params = degrade(pil_image, PRESETS["typical"])
"""

from __future__ import annotations

import argparse
import io
import json
from dataclasses import asdict, dataclass, replace
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

# The sensor frame the firmware is configured for (camera_esp.c: FRAMESIZE_SVGA).
SVGA = (800, 600)


@dataclass(frozen=True)
class DegradeParams:
    """Every knob, with units, so the output is fully reproducible."""

    width: int = SVGA[0]
    height: int = SVGA[1]
    # Gaussian optical blur radius in pixels (0 = perfectly sharp optics).
    blur_radius: float = 0.0
    # Motion blur: kernel length in pixels and angle in degrees (0 length = none).
    motion_len: int = 0
    motion_angle: float = 0.0
    # Low-light response: brightness gain (<1 darkens) and gamma (>1 darkens mids).
    light_gain: float = 1.0
    light_gamma: float = 1.0
    # Sensor noise: Gaussian sigma in 8-bit levels (0 = clean).
    noise_sigma: float = 0.0
    # JPEG re-encode quality (Pillow 1..95, higher = better). The OV5640's
    # quality=12 is aggressive; ~30 here is the rough perceptual equivalent.
    jpeg_quality: int = 30
    # Deterministic noise for reproducible runs/tests.
    seed: int = 0


# Named operating points, ordered from best to worst case.
PRESETS: dict[str, DegradeParams] = {
    # Resolution + compression only: the best the SVGA sensor could ever do.
    "clean": DegradeParams(jpeg_quality=40),
    # Good lighting, steady hand: mild blur + light compression.
    "good_light": DegradeParams(blur_radius=0.6, noise_sigma=2.0, jpeg_quality=35),
    # Motion-dominant: a sharp, well-lit frame ruined by hand/head movement —
    # isolates "what does motion blur alone cost the AI?".
    "motion_blur": DegradeParams(
        blur_radius=0.4,
        motion_len=14,
        motion_angle=25.0,
        noise_sigma=3.0,
        jpeg_quality=32,
    ),
    # The honest default: indoor, handheld — soft, a little motion, some noise.
    "typical": DegradeParams(
        blur_radius=1.0,
        motion_len=4,
        motion_angle=20.0,
        light_gain=0.85,
        light_gamma=1.15,
        noise_sigma=6.0,
        jpeg_quality=28,
    ),
    # Dim room: darker, much noisier, softer — where vision models start to fail.
    "low_light": DegradeParams(
        blur_radius=1.3,
        motion_len=6,
        motion_angle=30.0,
        light_gain=0.5,
        light_gamma=1.5,
        noise_sigma=14.0,
        jpeg_quality=22,
    ),
    # Realistic bad capture: walking, dim, cheap optics, heavy compression.
    "worst_case": DegradeParams(
        blur_radius=2.0,
        motion_len=12,
        motion_angle=45.0,
        light_gain=0.4,
        light_gamma=1.7,
        noise_sigma=22.0,
        jpeg_quality=16,
    ),
}


def _apply_motion(img: Image.Image, length: int, angle_deg: float) -> Image.Image:
    """Linear motion blur by averaging `length` copies shifted along `angle`.

    Mathematically equivalent to convolving with a line kernel, but vectorised
    (np.roll) so it runs in milliseconds on an SVGA frame instead of minutes.
    Small wrap-around at the borders is negligible for our purpose.
    """
    if length <= 1:
        return img
    arr = np.asarray(img, dtype=np.float32)
    rad = np.deg2rad(angle_deg)
    dx, dy = np.cos(rad), np.sin(rad)
    acc = np.zeros_like(arr)
    for i in range(length):
        t = i - (length - 1) / 2.0
        shift = (int(round(dy * t)), int(round(dx * t)))
        acc += np.roll(arr, shift, axis=(0, 1))
    acc /= length
    return Image.fromarray(np.clip(acc, 0, 255).astype(np.uint8))


def degrade(image: Image.Image, params: DegradeParams) -> tuple[Image.Image, dict]:
    """Return (degraded RGB JPEG-decoded image, params dict actually used).

    The returned image is what a downstream consumer (the AI provider) would
    receive: it has been through a real JPEG encode/decode at the chosen quality.
    """
    img = image.convert("RGB")

    # 1. Sensor resolution — fit within SVGA, preserving aspect (a real frame is
    #    a fixed window; we keep proportions and let the AI see the resolution it
    #    will actually get).
    img.thumbnail((params.width, params.height), Image.LANCZOS)

    # 2. Optical blur (soft/defocused cheap lens).
    if params.blur_radius > 0:
        img = img.filter(ImageFilter.GaussianBlur(params.blur_radius))

    # 3. Motion blur (hand/head movement during capture).
    img = _apply_motion(img, params.motion_len, params.motion_angle)

    # 4. Low-light response: linear gain then gamma (darkens midtones).
    arr = np.asarray(img, dtype=np.float32) / 255.0
    if params.light_gain != 1.0:
        arr *= params.light_gain
    if params.light_gamma != 1.0:
        arr = np.power(np.clip(arr, 0.0, 1.0), params.light_gamma)

    # 5. Sensor noise (Gaussian; worse in the dark, which is physically why
    #    low-light presets pair high noise with low gain).
    if params.noise_sigma > 0:
        rng = np.random.default_rng(params.seed)
        noise = rng.normal(0.0, params.noise_sigma / 255.0, arr.shape)
        arr = arr + noise
    arr = np.clip(arr * 255.0, 0, 255).astype(np.uint8)
    img = Image.fromarray(arr)

    # 6. JPEG compression — encode at the target quality and decode back, so the
    #    consumer sees real compression artifacts (blocking, chroma loss).
    buf = io.BytesIO()
    img.save(buf, format="JPEG", quality=params.jpeg_quality)
    jpeg_bytes = buf.getvalue()
    decoded = Image.open(io.BytesIO(jpeg_bytes)).convert("RGB")

    used = asdict(params)
    used["output_format"] = "image/jpeg"
    used["output_size_bytes"] = len(jpeg_bytes)
    used["output_resolution"] = list(decoded.size)
    return decoded, used


def degrade_file(in_path, out_path, params: DegradeParams) -> dict:
    """Degrade a file on disk; write the JPEG and a sidecar .params.json."""
    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    src = Image.open(in_path)
    decoded, used = degrade(src, params)
    decoded.save(out_path, format="JPEG", quality=params.jpeg_quality)
    sidecar = out_path.with_suffix(".params.json")
    sidecar.write_text(json.dumps(used, indent=2), encoding="utf-8")
    return used


def _params_from_args(args) -> DegradeParams:
    base = PRESETS[args.preset]
    overrides = {}
    if args.blur is not None:
        overrides["blur_radius"] = args.blur
    if args.noise is not None:
        overrides["noise_sigma"] = args.noise
    if args.jpeg_quality is not None:
        overrides["jpeg_quality"] = args.jpeg_quality
    if args.gain is not None:
        overrides["light_gain"] = args.gain
    if args.seed is not None:
        overrides["seed"] = args.seed
    return replace(base, **overrides)


def main() -> None:
    p = argparse.ArgumentParser(description="Degrade a photo toward OV5640/SVGA quality.")
    p.add_argument("input", help="source image (any format Pillow reads)")
    p.add_argument("--preset", choices=list(PRESETS), default="typical")
    p.add_argument("--out", help="output JPEG path (single preset)")
    p.add_argument("--all-presets", action="store_true", help="emit every preset")
    p.add_argument("--outdir", default="run/degraded", help="dir for --all-presets")
    p.add_argument("--blur", type=float, help="override Gaussian blur radius (px)")
    p.add_argument("--noise", type=float, help="override noise sigma (levels)")
    p.add_argument("--jpeg-quality", type=int, dest="jpeg_quality", help="Pillow 1..95")
    p.add_argument("--gain", type=float, help="override low-light gain (<1 darkens)")
    p.add_argument("--seed", type=int, help="noise RNG seed")
    args = p.parse_args()

    if args.all_presets:
        outdir = Path(args.outdir)
        for name in PRESETS:
            out = outdir / f"{Path(args.input).stem}.{name}.jpg"
            used = degrade_file(args.input, out, PRESETS[name])
            print(f"{name:11s} -> {out}  ({used['output_size_bytes']} B, "
                  f"{used['output_resolution']})")
    else:
        out = Path(args.out) if args.out else Path("run/degraded") / (
            f"{Path(args.input).stem}.{args.preset}.jpg"
        )
        used = degrade_file(args.input, out, _params_from_args(args))
        print(f"wrote {out} ({used['output_size_bytes']} B, {used['output_resolution']})")
        print(f"params -> {out.with_suffix('.params.json')}")


if __name__ == "__main__":
    main()
