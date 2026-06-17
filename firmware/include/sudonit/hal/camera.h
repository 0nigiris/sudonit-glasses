/* Sudonit HAL — camera.
 *
 * The seam between "capture an image" and "which sensor produced it". On the
 * host build this is backed by a mock that returns synthetic bytes; when the
 * OV5640 arrives, camera_esp.c implements the same four functions and nothing
 * above this header changes (see docs/HARDWARE_INTEGRATION_PLAN.md).
 */
#ifndef SUDONIT_HAL_CAMERA_H
#define SUDONIT_HAL_CAMERA_H

#include <stddef.h>
#include <stdint.h>

#include "sudonit/error.h"

typedef struct {
    const char *media_type; /* e.g. "image/jpeg"; mock uses "image/x-mock-rgb" */
    uint8_t *data;          /* owned by the HAL until sd_camera_release()      */
    size_t len;
    uint16_t width;
    uint16_t height;
} sd_image_t;

/* Prepare the camera. Safe to call once at startup. */
sd_err_t sd_camera_init(void);

/* Capture one frame. On SD_OK, `out` is filled and owns `data` until released. */
sd_err_t sd_camera_capture(sd_image_t *out);

/* Release the buffer from a successful capture. Safe on a zeroed struct. */
void sd_camera_release(sd_image_t *img);

/* Name of the active backend, for diagnostics/logs ("mock", "ov5640", ...). */
const char *sd_camera_backend(void);

#endif /* SUDONIT_HAL_CAMERA_H */
