# The `main` component compiles app_main plus all shared firmware sources.
#
# Sources are referenced by relative path from this directory so there is a
# single copy shared with the host build (../../src, ../../components). The
# ESP32 HAL backends (src/hal/esp32) are linked here instead of the host/mock
# ones. Keeping everything in one component avoids cross-component link plumbing;
# a finer component split is a future refactor (see PRE_HARDWARE_ROADMAP.md).

idf_component_register(
    SRCS
        "app_main.c"
        # core
        "../../src/core/error.c"
        "../../src/core/log.c"
        # configuration (shared module + NVS backend)
        "../../src/config/config.c"
        "../../src/config/config_store_nvs.c"
        # device application (hardware-agnostic)
        "../../src/app/device.c"
        # serial provisioning console (transport-independent command processor)
        "../../src/app/provisioning.c"
        # ESP32 HAL backends (stubs until drivers land)
        "../../src/hal/esp32/camera_esp.c"
        "../../src/hal/esp32/audio_esp.c"
        "../../src/hal/esp32/mic_esp.c"
        "../../src/hal/esp32/battery_esp.c"
        "../../src/hal/esp32/transport_wifi.c"
        "../../src/hal/esp32/net_esp.c"
        # protocol component sources
        "../../components/protocol/src/framing.c"
        "../../components/protocol/src/messages.c"
        "../../components/protocol/src/json.c"
        "../../components/protocol/src/sha256.c"
    INCLUDE_DIRS
        "."
        "../../include"
        "../../src/app"
        "../../components/protocol/include"
    PRIV_REQUIRES
        nvs_flash
        esp_wifi
        esp_netif
        esp_event)

# Optional bring-up/recovery features, off by default so production boots are
# unaffected. Enable from the command line, e.g.
#   idf.py build -DSUDONIT_NET_SELFTEST=1
#   idf.py build -DSUDONIT_PROVISION_CONSOLE=1
# These map a CMake cache variable to the preprocessor define app_main checks.
if(SUDONIT_NET_SELFTEST)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SUDONIT_NET_SELFTEST=1)
endif()
if(SUDONIT_PROVISION_CONSOLE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SUDONIT_PROVISION_CONSOLE=1)
endif()
