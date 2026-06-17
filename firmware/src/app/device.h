/* Sudonit device application — the hardware-agnostic glasses logic.
 *
 * This is the skeleton of what the firmware does each capture cycle, written
 * purely against the HAL. It runs identically on the host (mock backends) and
 * on the ESP32 (real backends) — only the linked drivers differ.
 */
#ifndef SUDONIT_APP_DEVICE_H
#define SUDONIT_APP_DEVICE_H

#include <stddef.h>
#include <stdint.h>

#include "sudonit/error.h"

/* Outcome of one capture cycle, for logging/tests/diagnostics. */
typedef struct {
    size_t image_bytes;
    uint16_t image_width;
    uint16_t image_height;
    uint8_t battery_percent;
} sd_device_status_t;

/* Initialize all HAL subsystems the device uses. */
sd_err_t sd_device_init(void);

/* Run one capture cycle: read battery, capture an image, release it.
 * (Transport + AI live on the phone; wiring this to the protocol component is
 * the next step — see PRE_HARDWARE_ROADMAP.md.) On SD_OK, `status` is filled.
 */
sd_err_t sd_device_capture_cycle(sd_device_status_t *status);

#endif /* SUDONIT_APP_DEVICE_H */
