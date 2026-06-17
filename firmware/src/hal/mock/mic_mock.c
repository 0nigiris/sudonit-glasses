/* Mock microphone backend (host build).
 *
 * Synthesizes a 440 Hz sine so the capture path returns plausible PCM without
 * an I2S mic. On hardware, mic_esp.c reads the ICS43434/INMP441 over I2S.
 */
#include "sudonit/hal/mic.h"

#include <math.h>

static uint32_t s_sample_rate;
static int s_initialized;
static double s_phase;

sd_err_t sd_mic_init(uint32_t sample_rate) {
    if (sample_rate == 0) {
        return SD_ERR_INVALID;
    }
    s_sample_rate = sample_rate;
    s_phase = 0.0;
    s_initialized = 1;
    return SD_OK;
}

sd_err_t sd_mic_read(int16_t *buf, size_t max_frames, size_t *frames_read) {
    if (!buf || !frames_read) {
        return SD_ERR_INVALID;
    }
    if (!s_initialized) {
        return SD_ERR_AGAIN;
    }
    const double freq = 440.0;
    const double step = 2.0 * M_PI * freq / (double)s_sample_rate;
    for (size_t i = 0; i < max_frames; ++i) {
        buf[i] = (int16_t)(sin(s_phase) * 8000.0);
        s_phase += step;
    }
    *frames_read = max_frames;
    return SD_OK;
}

const char *sd_mic_backend(void) { return "mock"; }
