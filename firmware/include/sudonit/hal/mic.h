/* Sudonit HAL — microphone input.
 *
 * Reads 16-bit PCM. Mock synthesizes a tone on the host; the real backend reads
 * the ICS43434/INMP441 over I2S. Used by the (future) voice uplink path.
 */
#ifndef SUDONIT_HAL_MIC_H
#define SUDONIT_HAL_MIC_H

#include <stddef.h>
#include <stdint.h>

#include "sudonit/error.h"

sd_err_t sd_mic_init(uint32_t sample_rate);

/* Read up to `max_frames` frames into `buf`; *frames_read gets the count. */
sd_err_t sd_mic_read(int16_t *buf, size_t max_frames, size_t *frames_read);

const char *sd_mic_backend(void);

#endif /* SUDONIT_HAL_MIC_H */
