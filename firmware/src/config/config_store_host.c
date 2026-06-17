/* Host configuration store: a single binary file.
 *
 * Path comes from SUDONIT_CONFIG_PATH (so tests and dev runs can isolate
 * state), defaulting to ./sudonit_config.bin. A missing file means "nothing
 * stored yet" — reported as *out_len == 0, not an error.
 */
#include "sudonit/config_store.h"

#include <stdio.h>
#include <stdlib.h>

static const char *config_path(void) {
    const char *p = getenv("SUDONIT_CONFIG_PATH");
    return (p && p[0]) ? p : "sudonit_config.bin";
}

sd_err_t sd_config_store_read(void *buf, size_t cap, size_t *out_len) {
    if (!buf || !out_len) {
        return SD_ERR_INVALID;
    }
    *out_len = 0;

    FILE *f = fopen(config_path(), "rb");
    if (!f) {
        return SD_OK; /* no file yet — fresh device */
    }
    size_t n = fread(buf, 1, cap, f);
    int overflow = (n == cap) && (fgetc(f) != EOF); /* more data than fits */
    fclose(f);
    if (overflow) {
        return SD_ERR_NO_MEM;
    }
    *out_len = n;
    return SD_OK;
}

sd_err_t sd_config_store_write(const void *buf, size_t len) {
    if (!buf && len > 0) {
        return SD_ERR_INVALID;
    }
    FILE *f = fopen(config_path(), "wb");
    if (!f) {
        return SD_ERR_IO;
    }
    size_t n = fwrite(buf, 1, len, f);
    fclose(f);
    return (n == len) ? SD_OK : SD_ERR_IO;
}
