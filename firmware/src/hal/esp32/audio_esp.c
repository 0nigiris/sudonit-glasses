/* ESP32 audio-output backend — STUB (no hardware behavior).
 *
 * The real driver drives the MAX98357A over I2S. This stub does NOT emit sound:
 * play returns SD_ERR_UNSUPPORTED until the I2S driver lands. See
 * docs/HARDWARE_INTEGRATION_PLAN.md section 3.
 */
#include "sudonit/hal/audio.h"

sd_err_t sd_audio_init(uint32_t sample_rate, uint8_t channels) {
    if (sample_rate == 0 || channels == 0) {
        return SD_ERR_INVALID;
    }
    return SD_OK; /* no-op init; real init configures the I2S TX channel */
}

sd_err_t sd_audio_play_pcm(const int16_t *samples, size_t frame_count) {
    (void)samples;
    (void)frame_count;
    return SD_ERR_UNSUPPORTED; /* TODO: I2S write to MAX98357A */
}

const char *sd_audio_backend(void) { return "max98357a (stub)"; }
