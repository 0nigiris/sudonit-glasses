/* Mock audio-output backend (host build).
 *
 * Doesn't touch a sound device — it validates arguments and tallies how many
 * frames were "played", which is what tests assert on. On hardware,
 * audio_esp.c drives the MAX98357A over I2S.
 */
#include "sudonit/hal/audio.h"

static int s_initialized;
static uint32_t s_sample_rate;
static uint8_t s_channels;
static size_t s_frames_played; /* exposed for tests via the accessor below */

sd_err_t sd_audio_init(uint32_t sample_rate, uint8_t channels) {
    if (sample_rate == 0 || channels == 0) {
        return SD_ERR_INVALID;
    }
    s_sample_rate = sample_rate;
    s_channels = channels;
    s_frames_played = 0;
    s_initialized = 1;
    return SD_OK;
}

sd_err_t sd_audio_play_pcm(const int16_t *samples, size_t frame_count) {
    if (!s_initialized) {
        return SD_ERR_AGAIN;
    }
    if (!samples && frame_count > 0) {
        return SD_ERR_INVALID;
    }
    s_frames_played += frame_count;
    return SD_OK;
}

const char *sd_audio_backend(void) { return "mock"; }

/* Test/diagnostics hook — not part of the public HAL contract. */
size_t sd_audio_mock_frames_played(void) { return s_frames_played; }
