/* ESP32 microphone backend — STUB (no hardware behavior).
 *
 * The real driver reads the ICS43434/INMP441 over I2S. This stub does NOT
 * synthesize audio: read returns SD_ERR_UNSUPPORTED until the I2S RX driver
 * lands. See docs/HARDWARE_INTEGRATION_PLAN.md section 4.
 */
#include "sudonit/hal/mic.h"

sd_err_t sd_mic_init(uint32_t sample_rate) {
    if (sample_rate == 0) {
        return SD_ERR_INVALID;
    }
    return SD_OK; /* no-op init; real init configures the I2S RX channel */
}

sd_err_t sd_mic_read(int16_t *buf, size_t max_frames, size_t *frames_read) {
    (void)buf;
    (void)max_frames;
    (void)frames_read;
    return SD_ERR_UNSUPPORTED; /* TODO: I2S read from the microphone */
}

const char *sd_mic_backend(void) { return "ics43434 (stub)"; }
