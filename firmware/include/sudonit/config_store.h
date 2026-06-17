/* Storage backend interface for the configuration subsystem (internal).
 *
 * A tiny blob store: the config module owns serialization + versioning; the
 * backend just persists and returns an opaque blob. Two implementations:
 *   - config_store_host.c : a file on disk (host build)
 *   - config_store_nvs.c  : NVS (ESP-IDF build)
 *
 * This is the config-system's hardware seam, mirroring the HAL pattern.
 */
#ifndef SUDONIT_CONFIG_STORE_H
#define SUDONIT_CONFIG_STORE_H

#include <stddef.h>

#include "sudonit/error.h"

/* Read the stored blob into `buf` (up to `cap`). On SD_OK, *out_len is the
 * number of bytes read; *out_len == 0 means "nothing stored yet" (not an error).
 * SD_ERR_IO on backend failure; SD_ERR_NO_MEM if the blob exceeds `cap`.
 */
sd_err_t sd_config_store_read(void *buf, size_t cap, size_t *out_len);

/* Persist `len` bytes from `buf`, replacing any previous blob. */
sd_err_t sd_config_store_write(const void *buf, size_t len);

#endif /* SUDONIT_CONFIG_STORE_H */
