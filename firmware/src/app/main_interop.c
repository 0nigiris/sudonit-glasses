/* Interop demo: the host-built firmware talks to the real Python phone server.
 *
 *   capture (mock camera) -> send via transport -> Python phone server
 *   -> AI response -> back into the firmware app layer (printed here).
 *
 * Requires `python -m phone.server` running (see firmware/test/run_interop.sh).
 * Usage: device_interop [host] [port]
 */
#include <stdio.h>
#include <stdlib.h>

#include "device.h"
#include "sudonit/hal/transport.h"
#include "sudonit/log.h"

int main(int argc, char **argv) {
    const char *host = (argc > 1) ? argv[1] : "127.0.0.1";
    uint16_t port = (uint16_t)((argc > 2) ? atoi(argv[2]) : 8765);

    sd_log_set_level(SD_LOG_INFO);

    if (sd_device_init() != SD_OK) {
        return 1;
    }

    sd_transport_t *t = NULL;
    if (sd_transport_connect(&t, host, port) != SD_OK) {
        fprintf(stderr, "could not connect to phone at %s:%u — is "
                        "`python -m phone.server` running?\n", host, port);
        return 1;
    }

    char response[1024];
    sd_err_t err = sd_device_run_uplink(t, "capture", response, sizeof(response));
    sd_transport_close(t);

    if (err != SD_OK) {
        fprintf(stderr, "uplink failed: %s\n", sd_strerror(err));
        return 1;
    }

    printf("PHONE RESPONSE: %s\n", response);
    return 0;
}
