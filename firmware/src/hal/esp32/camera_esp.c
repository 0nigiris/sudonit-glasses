/* ESP32 camera backend — STUB (no hardware behavior).
 *
 * The OV5640 driver (DVP + esp32-camera, JPEG capture into PSRAM) is not
 * implemented yet. Per project rules this stub does NOT fake a frame: capture
 * returns SD_ERR_UNSUPPORTED until the real driver lands. See
 * docs/HARDWARE_INTEGRATION_PLAN.md section 2.
 */
#include "sudonit/hal/camera.h"

sd_err_t sd_camera_init(void) {
    return SD_OK; /* no-op init; real init configures the OV5640 over SCCB/DVP */
}

sd_err_t sd_camera_capture(sd_image_t *out) {
    (void)out;
    return SD_ERR_UNSUPPORTED; /* TODO: OV5640 capture */
}

void sd_camera_release(sd_image_t *img) {
    (void)img; /* nothing allocated by the stub */
}

const char *sd_camera_backend(void) { return "ov5640 (stub)"; }
