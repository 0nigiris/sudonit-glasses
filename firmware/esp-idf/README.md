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
| camera       | `src/hal/esp32/camera_esp.c`     | stub — OV5640 driver TODO      |
| audio        | `src/hal/esp32/audio_esp.c`      | stub — MAX98357A I2S TODO      |
| mic          | `src/hal/esp32/mic_esp.c`        | stub — ICS43434 I2S TODO       |
| battery      | `src/hal/esp32/battery_esp.c`    | stub — VBAT ADC TODO           |
| transport    | `src/hal/esp32/transport_wifi.c` | stub — Wi-Fi STA + TCP TODO    |

Each stub returns `SD_ERR_UNSUPPORTED` (no fake hardware behavior). Implementing
one real driver behind its existing header is the per-peripheral integration
step in `../../docs/HARDWARE_INTEGRATION_PLAN.md`.
