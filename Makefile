# Sudonit — developer build automation.
#
# One-command build + test for the host stack, plus thin wrappers around the
# ESP-IDF build. Mirrors exactly what CI runs (.github/workflows/ci.yml), so a
# green `make ci` locally means a green pipeline.
#
#   make            # build the firmware host target, then run all tests
#   make help       # list every target
#
# No hardware and no API key are required for any host target.

FIRMWARE_DIR := firmware
BUILD_DIR    := $(FIRMWARE_DIR)/build
ESP_DIR      := $(FIRMWARE_DIR)/esp-idf
PYTHON       ?= python3

# Quiet, repeatable: default goal builds then tests the whole stack.
.DEFAULT_GOAL := all
.PHONY: all ci build configure ctest pytest test eval deps clean esp32 esp32-uplink lint help

## all: build the host firmware and run C + Python tests (default)
all: build test

## ci: exactly what GitHub Actions runs — configure, build, ctest, pytest
ci: configure build ctest pytest

## deps: install the Python dev/test dependencies
deps:
	$(PYTHON) -m pip install -r requirements-dev.txt

## configure: generate the host CMake build (idempotent)
configure:
	cmake -S $(FIRMWARE_DIR) -B $(BUILD_DIR)

## build: compile the firmware host target (mock HAL, -Werror)
build: configure
	cmake --build $(BUILD_DIR)

## ctest: run the C unit + robustness tests
ctest: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

## pytest: run the Python test suite (offline stub, no API key)
pytest:
	$(PYTHON) -m pytest -q

## test: run both C (ctest) and Python (pytest) tests
test: ctest pytest

## eval: offline smoke run of the Claude eval harness (stub provider, no key)
eval:
	$(PYTHON) -m eval.run_eval --provider stub --variants original --limit 1

## lint: run the same lint + static analysis as CI (needs ruff + cppcheck)
lint:
	ruff check phone/ protocol/ eval/ tools/ tests/ firmware/
	cppcheck --enable=warning,portability --error-exitcode=1 --inline-suppr \
		--quiet --suppress=missingIncludeSystem \
		--suppress=normalCheckLevelMaxBranches \
		-I firmware/include -I firmware/components/protocol/include \
		-I firmware/src/app \
		firmware/src firmware/components/protocol firmware/test \
		firmware/esp-idf/main/app_main.c

## clean: remove the host build directory
clean:
	rm -rf $(BUILD_DIR)

## esp32: build the firmware for ESP32-S3 (requires a sourced ESP-IDF env)
esp32:
	idf.py -C $(ESP_DIR) set-target esp32s3
	idf.py -C $(ESP_DIR) build

## esp32-uplink: ESP32-S3 build with the full capture->AI->audio loop wired
esp32-uplink:
	idf.py -C $(ESP_DIR) build -DSUDONIT_RUN_UPLINK=1 -DSUDONIT_NET_SELFTEST=1

## help: list available targets
help:
	@grep -E '^## ' $(MAKEFILE_LIST) | sed 's/^## /  /'
