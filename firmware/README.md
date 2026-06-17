# Sudonit firmware

The ESP32-S3 firmware, written so the entire stack above the hardware line can
be built and tested **before** any hardware arrives.

## The idea: a Hardware Abstraction Layer (HAL)

Every peripheral sits behind a small interface in `include/sudonit/hal/`:

| Interface         | Host backend (now)          | ESP32 backend (on arrival)   |
|-------------------|-----------------------------|------------------------------|
| `hal/camera.h`    | `src/hal/mock/camera_mock.c` | OV5640 via esp32-camera      |
| `hal/audio.h`     | `src/hal/mock/audio_mock.c`  | MAX98357A over I2S           |
| `hal/mic.h`       | `src/hal/mock/mic_mock.c`    | ICS43434/INMP441 over I2S    |
| `hal/battery.h`   | `src/hal/mock/battery_mock.c`| ADC read of the VBAT divider |

The device logic (`src/app/device.c`) is written purely against these
interfaces, so it runs identically on the host (mocks) and on the ESP32 (real
drivers). Integrating a peripheral = implementing one `*_esp.c` file behind an
existing header. See `../docs/HARDWARE_INTEGRATION_PLAN.md`.

## Layout

```
firmware/
├── include/sudonit/        # public interfaces (error, log, hal/*)
├── src/
│   ├── core/               # error strings, logging seam
│   ├── hal/mock/           # host mock drivers (swapped for hal/esp32 later)
│   └── app/                # device loop (hardware-agnostic) + host main
├── test/                   # host tests for the HAL + device loop
└── CMakeLists.txt          # host build (gcc/clang); ESP-IDF build added later
```

## Build & test (host, no hardware)

```bash
cmake -S firmware -B firmware/build
cmake --build firmware/build
ctest --test-dir firmware/build --output-on-failure
./firmware/build/device_demo        # run a few mock capture cycles
```

## When hardware arrives

Add `src/hal/esp32/{camera,audio,mic,battery}_esp.c` implementing the same
headers, plus an ESP-IDF build that sets `SUDONIT_HAL_BACKEND=esp32`. No change
to `src/app/` or `include/`.
