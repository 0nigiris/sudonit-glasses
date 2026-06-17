# Host build for the Sudonit firmware HAL + app skeleton.
#
# This builds the hardware-agnostic firmware code (HAL interfaces, mock drivers,
# device loop) on a normal dev machine with gcc/clang — no ESP-IDF, no silicon.
# It lets the whole firmware stack be compiled and tested before hardware.
#
# When hardware arrives, an ESP-IDF build is added alongside this (a second
# CMake entry under the ESP-IDF build system) that compiles the same src/app
# and src/hal interfaces but links src/hal/esp32/*.c instead of src/hal/mock/*.c.
#
#   cmake -S firmware -B firmware/build
#   cmake --build firmware/build
#   ctest --test-dir firmware/build --output-on-failure

cmake_minimum_required(VERSION 3.16)
project(sudonit_firmware C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror)

# Backend selection: the host build links the mock HAL. The ESP-IDF build will
# point this at src/hal/esp32 instead — same headers, different drivers.
set(SUDONIT_HAL_BACKEND "mock" CACHE STRING "HAL backend: mock | esp32")

set(CORE_SRC
    src/core/error.c
    src/core/log.c)

set(HAL_SRC
    src/hal/${SUDONIT_HAL_BACKEND}/camera_${SUDONIT_HAL_BACKEND}.c
    src/hal/${SUDONIT_HAL_BACKEND}/battery_${SUDONIT_HAL_BACKEND}.c
    src/hal/${SUDONIT_HAL_BACKEND}/audio_${SUDONIT_HAL_BACKEND}.c
    src/hal/${SUDONIT_HAL_BACKEND}/mic_${SUDONIT_HAL_BACKEND}.c)

add_library(sudonit_fw STATIC ${CORE_SRC} ${HAL_SRC} src/app/device.c)
target_include_directories(sudonit_fw PUBLIC include src/app)
# mic_mock uses sin(); link libm where required.
target_link_libraries(sudonit_fw m)

add_executable(device_demo src/app/main_host.c)
target_link_libraries(device_demo sudonit_fw)

enable_testing()
add_executable(test_hal test/test_hal.c)
target_link_libraries(test_hal sudonit_fw)
add_test(NAME hal COMMAND test_hal)
