/* Interop demo: the host-built firmware talks to the real Python phone server.
 *
 *   capture (mock camera) -> send via transport -> Python phone server
 *   -> AI response -> back into the firmware app layer (printed here).
 *
 * The server address comes from the device configuration (the same source the
 * ESP32 reads from NVS) — provision it with `device_config set server_host ...`.
 * Optional CLI args still override, for ad-hoc runs.
 *
 * Requires `python -m phone.server` running (see firmware/test/run_interop.sh).
 * Usage: device_interop [host] [port]
 */
#include <stdio.h>
#include <stdlib.h>

#include "device.h"
#include "sudonit/config.h"
#include "sudonit/hal/transport.h"
#include "sudonit/log.h"

int main(int argc, char **argv) {
    sd_log_set_level(SD_LOG_INFO);

    /* Consume configuration instead of hardcoded values. CLI args override. */
    sd_config_t cfg;
    sd_config_load(&cfg);
    const char *host = (argc > 1) ? argv[1]
                       : (cfg.server_host[0] ? cfg.server_host : "127.0.0.1");
    uint16_t port = (uint16_t)((argc > 2) ? atoi(argv[2]) : cfg.server_port);

    SD_LOGI("interop", "device=%s server=%s:%u", cfg.device_name, host,
            (unsigned)port);

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
