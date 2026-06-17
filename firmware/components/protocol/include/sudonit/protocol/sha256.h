/* SHA-256 for the protocol layer.
 *
 * The phone verifies the SHA-256 declared in `image_begin` and rejects a
 * mismatch, so the firmware must compute the identical digest. ESP-IDF ships
 * mbedtls SHA-256; this small portable implementation is used by the host build
 * (and as a fallback) so the protocol component has no external dependency.
 */
#ifndef SUDONIT_PROTOCOL_SHA256_H
#define SUDONIT_PROTOCOL_SHA256_H

#include <stddef.h>
#include <stdint.h>

/* Compute the raw 32-byte SHA-256 digest of `data`. */
void sd_sha256(const uint8_t *data, size_t len, uint8_t out[32]);

/* Compute the lowercase hex digest (matches Python hashlib.hexdigest()).
 * `out_hex` must hold at least 65 bytes (64 hex chars + NUL). */
void sd_sha256_hex(const uint8_t *data, size_t len, char out_hex[65]);

#endif /* SUDONIT_PROTOCOL_SHA256_H */
