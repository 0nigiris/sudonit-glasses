# ESP-IDF project root for the Sudonit firmware (target: ESP32-S3).
#
# This is the on-target build. It reuses the exact same shared sources as the
# host build (HAL interfaces, protocol component, device app) — see the main
# component's CMakeLists. The host build lives at ../CMakeLists.txt and is
# unaffected by this project.
#
#   . $IDF_PATH/export.sh
#   idf.py -C firmware/esp-idf set-target esp32s3
#   idf.py -C firmware/esp-idf build

cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sudonit_glasses)
