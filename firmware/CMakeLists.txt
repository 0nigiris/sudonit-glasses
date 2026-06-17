# Host build for the Sudonit firmware: HAL + protocol + app skeleton.
#
# Builds the hardware-agnostic firmware on a normal dev machine with gcc/clang —
# no ESP-IDF, no silicon — so the whole stack is compiled and tested before
# hardware. When hardware arrives, an ESP-IDF build is added alongside this that
# compiles the same sources but links the esp32 HAL backends and the Wi-Fi
# transport instead of the host/mock ones.
#
#   cmake -S firmware -B firmware/build
#   cmake --build firmware/build
#   ctest --test-dir firmware/build --output-on-failure
#   ./firmware/build/device_demo                 # mock capture cycles
#   ./firmware/test/run_interop.sh               # live interop vs Python phone

cmake_minimum_required(VERSION 3.16)
project(sudonit_firmware C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror)

# HAL backend: the host build uses mock peripheral drivers + a TCP transport.
# The ESP-IDF build will select the esp32 backends instead (same headers).
set(SUDONIT_HAL_BACKEND "mock" CACHE STRING "HAL backend: mock | esp32")

# --- core + config + HAL (mock peripherals, host TCP transport) ---
set(CORE_SRC
    src/core/error.c
    src/core/log.c
    src/config/config.c
    src/config/config_store_host.c)

set(HAL_SRC
    src/hal/mock/camera_mock.c
    src/hal/mock/battery_mock.c
    src/hal/mock/audio_mock.c
    src/hal/mock/mic_mock.c
    src/hal/host/transport_tcp.c)

# --- protocol component (C port; same sources the ESP-IDF component uses) ---
set(PROTOCOL_SRC
    components/protocol/src/framing.c
    components/protocol/src/messages.c
    components/protocol/src/json.c
    components/protocol/src/sha256.c)

add_library(sudonit_fw STATIC
    ${CORE_SRC} ${HAL_SRC} ${PROTOCOL_SRC} src/app/device.c)
target_include_directories(sudonit_fw PUBLIC
    include
    src/app
    components/protocol/include)
target_link_libraries(sudonit_fw m)

# --- executables ---
add_executable(device_demo src/app/main_host.c)
target_link_libraries(device_demo sudonit_fw)

add_executable(device_interop src/app/main_interop.c)
target_link_libraries(device_interop sudonit_fw)

add_executable(device_config src/app/main_config.c)
target_link_libraries(device_config sudonit_fw)

# --- tests ---
enable_testing()

add_executable(test_hal test/test_hal.c)
target_link_libraries(test_hal sudonit_fw)
add_test(NAME hal COMMAND test_hal)

find_package(Threads REQUIRED)
add_executable(test_protocol test/test_protocol.c)
target_link_libraries(test_protocol sudonit_fw Threads::Threads)
add_test(NAME protocol COMMAND test_protocol)

add_executable(test_config test/test_config.c)
target_link_libraries(test_config sudonit_fw)
add_test(NAME config COMMAND test_config)
