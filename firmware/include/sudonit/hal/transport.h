/* Sudonit HAL — transport (the data-plane link to the phone).
 *
 * A reliable, ordered byte stream. On the host this is a TCP socket (the Wi-Fi
 * data-plane stand-in, per DECISIONS.md / protocol/TRANSPORT.md); on the ESP32
 * it will be a Wi-Fi TCP connection implementing the same four functions.
 *
 * send/recv are all-or-nothing: they move exactly `len` bytes or return an
 * error. That matches what the framing layer needs and keeps it simple.
 */
#ifndef SUDONIT_HAL_TRANSPORT_H
#define SUDONIT_HAL_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

#include "sudonit/error.h"

typedef struct sd_transport sd_transport_t; /* opaque; defined by the backend */

/* Open a connection. On SD_OK, *out is a handle to close with sd_transport_close. */
sd_err_t sd_transport_connect(sd_transport_t **out, const char *host, uint16_t port);

/* Send exactly `len` bytes (handles partial writes internally). */
sd_err_t sd_transport_send(sd_transport_t *t, const uint8_t *buf, size_t len);

/* Receive exactly `len` bytes (blocks until filled). SD_ERR_IO if peer closes. */
sd_err_t sd_transport_recv(sd_transport_t *t, uint8_t *buf, size_t len);

void sd_transport_close(sd_transport_t *t);

/* Host/test-only: wrap an already-connected fd (e.g. one end of a socketpair)
 * so framing can be unit-tested in-process. Not part of the on-target API. */
sd_transport_t *sd_transport_wrap_fd(int fd);

#endif /* SUDONIT_HAL_TRANSPORT_H */
