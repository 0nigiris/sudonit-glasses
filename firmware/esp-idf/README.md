# Sudonit firmware — ESP-IDF target build (ESP32-S3)

The on-target build. It compiles the **same** shared sources as the host build
(`../`): the HAL interfaces, the protocol component, and the device app. The
difference is the linked backends — this build uses `src/hal/esp32/*.c` (real
peripheral drivers, currently honest stubs) instead of the host mock/TCP ones.

## Build

```bash
. $IDF_PATH/export.sh                       # e.g. ~/esp/esp-idf/export.sh
idf.py -C firmware/esp-idf set-target esp32s3
idf.py -C firmware/esp-idf build
```

## Flash & monitor (with hardware)

```bash
idf.py -C firmware/esp-idf -p /dev/ttyACM0 flash monitor
```

`app_main()` logs the boot banner, the active HAL backends, and the result of a
capture cycle. Until the real drivers land, the peripheral stubs report
`unsupported` — that is expected and proves the app→HAL path runs on target.

## Optional bring-up features (off by default)

Two build-time features are compiled out of the default (production) build and
enabled with a CMake flag:

```bash
# Wi-Fi data-plane self-test: bring up Wi-Fi STA from the provisioned creds,
# connect to the phone server, and exchange ping/pong (proves net_esp +
# transport_wifi + protocol end-to-end). Provision first with device_provision.
idf.py -C firmware/esp-idf build -DSUDONIT_NET_SELFTEST=1

# Serial provisioning/recovery console over the UART (show/get/set/save/reset).
idf.py -C firmware/esp-idf build -DSUDONIT_PROVISION_CONSOLE=1

# Real OV5640 camera driver (esp32-camera). Off by default; see
# docs/CAMERA_BRINGUP.md for the pin map to verify and the day-one checklist.
idf.py -C firmware/esp-idf build -DSUDONIT_CAMERA_DRIVER=1

# Real MAX98357A I2S audio-output driver. Off by default; see
# docs/AUDIO_BRINGUP.md for the I2S pin map / camera pin-conflict analysis.
idf.py -C firmware/esp-idf build -DSUDONIT_AUDIO_DRIVER=1
```

The self-test uses `ping`/`pong` rather than a full image uplink so it does not
depend on the (still-stubbed) camera; once the camera driver lands, the same
path runs `sd_device_run_uplink` with no change to the protocol layer.

## Simulate (Wokwi, no hardware)

Boot + serial logs in the Wokwi ESP32-S3 simulator. Requires the Wokwi CLI and a
free token:

```bash
curl -L https://wokwi.com/ci/install.sh | sh
export WOKWI_CLI_TOKEN=...                   # from https://wokwi.com
idf.py -C firmware/esp-idf build
wokwi-cli firmware/esp-idf                   # uses wokwi.toml + diagram.json
```

## Layout

```
firmware/esp-idf/
├── CMakeLists.txt        # IDF project root (project: sudonit_glasses)
├── sdkconfig.defaults    # 16MB flash, octal PSRAM, custom partitions
├── partitions.csv        # single factory app (OTA is a future subsystem)
├── wokwi.toml            # simulator config (scaffolding)
├── diagram.json          # simulator board (ESP32-S3 DevKitC-1)
└── main/
    ├── CMakeLists.txt     # registers app_main + all shared sources
    └── app_main.c         # ESP-IDF entry point
```

## Backends in this build

| HAL          | Backend file                    | State                          |
|--------------|----------------------------------|--------------------------------|
| camera       | `src/hal/esp32/camera_esp.c`     | OV5640 driver written (behind `-DSUDONIT_CAMERA_DRIVER=1`); default build is a stub. See `docs/CAMERA_BRINGUP.md` |
| audio        | `src/hal/esp32/audio_esp.c`      | MAX98357A I2S driver written (behind `-DSUDONIT_AUDIO_DRIVER=1`); default build is a stub. See `docs/AUDIO_BRINGUP.md` |
| mic          | `src/hal/esp32/mic_esp.c`        | stub — ICS43434 I2S TODO       |
| battery      | `src/hal/esp32/battery_esp.c`    | stub — VBAT ADC TODO           |
| transport    | `src/hal/esp32/transport_wifi.c` | **implemented** — LwIP TCP client |
| net          | `src/hal/esp32/net_esp.c`        | **implemented** — Wi-Fi STA bring-up |

Full board pin/PSRAM allocation and the consolidated day-one bring-up checklist
live in `../../docs/BOARD_RESOURCES.md` (the single source of truth).

The peripheral stubs return `SD_ERR_UNSUPPORTED` (no fake hardware behavior).
Implementing one real driver behind its existing header is the per-peripheral
integration step in `../../docs/HARDWARE_INTEGRATION_PLAN.md`. The transport and
net backends are implemented (compiled & linked); what remains is on-silicon
verification against a live AP and phone server.
