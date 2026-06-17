/* ESP32 transport backend — STUB (no hardware behavior).
 *
 * The real backend brings up Wi-Fi STA and opens a TCP connection to the phone
 * (the data plane, per protocol/TRANSPORT.md). It needs network credentials and
 * a server address from the (not-yet-built) config system, so this stub does
 * NOT fake a connection: connect/send/recv return SD_ERR_UNSUPPORTED until the
 * Wi-Fi driver + config land. See docs/HARDWARE_INTEGRATION_PLAN.md section 1.
 */
#include "sudonit/hal/transport.h"

struct sd_transport {
    int unused;
};

sd_err_t sd_transport_connect(sd_transport_t **out, const char *host, uint16_t port) {
    (void)host;
    (void)port;
    if (out) {
        *out = NULL;
    }
    return SD_ERR_UNSUPPORTED; /* TODO: Wi-Fi STA + TCP connect */
}

sd_err_t sd_transport_send(sd_transport_t *t, const uint8_t *buf, size_t len) {
    (void)t;
    (void)buf;
    (void)len;
    return SD_ERR_UNSUPPORTED;
}

sd_err_t sd_transport_recv(sd_transport_t *t, uint8_t *buf, size_t len) {
    (void)t;
    (void)buf;
    (void)len;
    return SD_ERR_UNSUPPORTED;
}

void sd_transport_close(sd_transport_t *t) {
    (void)t;
}
