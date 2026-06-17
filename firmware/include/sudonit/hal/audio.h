/* Sudonit HAL — audio output.
 *
 * Plays 16-bit PCM. Mock counts frames (and remembers the last buffer) on the
 * host; the real backend drives the MAX98357A over I2S. Keeping the glasses
 * "dumb" (phone renders speech, glasses just play PCM) is the intended path.
 */
#ifndef SUDONIT_HAL_AUDIO_H
#define SUDONIT_HAL_AUDIO_H

#include <stddef.h>
#include <stdint.h>

#include "sudonit/error.h"

sd_err_t sd_audio_init(uint32_t sample_rate, uint8_t channels);

/* Play `frame_count` interleaved 16-bit PCM frames. Blocks until queued. */
sd_err_t sd_audio_play_pcm(const int16_t *samples, size_t frame_count);

const char *sd_audio_backend(void);

#endif /* SUDONIT_HAL_AUDIO_H */
