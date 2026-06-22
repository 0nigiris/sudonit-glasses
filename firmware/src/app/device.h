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
#include "sudonit/hal/transport.h"

/* Outcome of one capture cycle, for logging/tests/diagnostics. */
typedef struct {
    size_t image_bytes;
    uint16_t image_width;
    uint16_t image_height;
    uint8_t battery_percent;
} sd_device_status_t;

/* Per-stage timing and sizes for one uplink turn. Filled by
 * sd_device_run_uplink when a non-NULL pointer is passed, so the host loop (and
 * the device on hardware) can report real latency without a profiler. All times
 * are wall-clock milliseconds from a monotonic clock. */
typedef struct {
    uint32_t capture_ms;     /* camera capture */
    uint32_t upload_ms;      /* streaming the image to the phone */
    uint32_t response_ms;    /* upload-done -> end of turn (AI + audio downlink) */
    uint32_t total_ms;       /* whole turn, capture through playback */
    size_t image_bytes;      /* bytes streamed to the phone */
    size_t response_bytes;   /* length of the AI text answer */
} sd_uplink_metrics_t;

/* Initialize all HAL subsystems the device uses. */
sd_err_t sd_device_init(void);

/* Run one capture cycle: read battery, capture an image, release it.
 * (Transport + AI live on the phone; wiring this to the protocol component is
 * the next step — see PRE_HARDWARE_ROADMAP.md.) On SD_OK, `status` is filled.
 */
sd_err_t sd_device_capture_cycle(sd_device_status_t *status);

/* Full V1 uplink over an open transport: capture an image (mock camera),
 * stream it to the phone via the protocol, wait for the response, and hand the
 * AI text back to the application layer in `response_out` (NUL-terminated,
 * truncated to `response_cap`). This is the firmware half of the core loop,
 * proven against the real Python phone server with no ESP32 hardware.
 *
 * If `metrics` is non-NULL it is filled with per-stage latency and sizes for the
 * turn (see sd_uplink_metrics_t); pass NULL to skip instrumentation.
 */
sd_err_t sd_device_run_uplink(sd_transport_t *t, const char *image_id,
                              char *response_out, size_t response_cap,
                              sd_uplink_metrics_t *metrics);

#endif /* SUDONIT_APP_DEVICE_H */
