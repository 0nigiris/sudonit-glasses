/* Sudonit HAL — network bring-up (getting the device onto the data-plane LAN).
 *
 * This is deliberately separate from the transport (sudonit/hal/transport.h):
 * the transport opens a TCP connection, while this one-time step brings the
 * network stack up so a connection is possible at all. On the ESP32 that means
 * associating with the configured Wi-Fi access point (STA) and obtaining an IP;
 * on the host the OS already provides networking, so the backend is a no-op.
 *
 * Keeping the two apart mirrors reality (the host transport knows nothing about
 * Wi-Fi) and keeps each backend single-purpose.
 */
#ifndef SUDONIT_HAL_NET_H
#define SUDONIT_HAL_NET_H

#include "sudonit/config.h"
#include "sudonit/error.h"

/* Bring the network up using the provided configuration (Wi-Fi SSID/password on
 * the ESP32). Blocks until the link is usable (an IP is assigned) or fails with
 * a timeout. SD_OK means a subsequent sd_transport_connect can be attempted.
 * The host backend returns SD_OK immediately. */
sd_err_t sd_net_start(const sd_config_t *cfg);

/* Tear the network down (disconnect Wi-Fi). Safe to call if never started. */
void sd_net_stop(void);

/* Human-readable backend name ("host" | "esp32"), for boot logging. */
const char *sd_net_backend(void);

#endif /* SUDONIT_HAL_NET_H */
