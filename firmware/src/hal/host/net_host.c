/* Host network backend — no-op.
 *
 * On a dev machine the operating system already provides networking, so there
 * is nothing to bring up: sd_net_start succeeds immediately and the transport
 * (transport_tcp.c) connects over the existing stack. This exists only so the
 * shared app code can call sd_net_start unconditionally on both builds.
 */
#include "sudonit/hal/net.h"

sd_err_t sd_net_start(const sd_config_t *cfg) {
    (void)cfg; /* the OS owns the network on the host */
    return SD_OK;
}

void sd_net_stop(void) {
    /* nothing to tear down */
}

const char *sd_net_backend(void) {
    return "host";
}
