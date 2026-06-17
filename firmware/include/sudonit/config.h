/* Sudonit firmware — configuration.
 *
 * Device configuration shared by the host and ESP-IDF builds. The same API and
 * serialization run everywhere; only the storage backend differs (a file on the
 * host, NVS on the ESP32). This is what the Wi-Fi transport will read for the
 * server address and network credentials.
 *
 * Versioned: SD_CONFIG_VERSION is stored alongside the data so a future schema
 * change can migrate (or fall back to defaults) instead of reading garbage.
 */
#ifndef SUDONIT_CONFIG_H
#define SUDONIT_CONFIG_H

#include <stddef.h>
#include <stdint.h>

#include "sudonit/error.h"

/* Bump when the sd_config_t layout/semantics change; load() then migrates or
 * falls back to defaults rather than trusting an incompatible blob. */
#define SD_CONFIG_VERSION 1u

/* Field capacities (excluding the NUL terminator). Wi-Fi limits are the 802.11
 * maxima: 32-char SSID, 63-char PSK. */
#define SD_WIFI_SSID_MAX 32
#define SD_WIFI_PASS_MAX 63
#define SD_HOST_MAX 63
#define SD_NAME_MAX 32

typedef struct {
    char wifi_ssid[SD_WIFI_SSID_MAX + 1];
    char wifi_password[SD_WIFI_PASS_MAX + 1];
    char server_host[SD_HOST_MAX + 1];
    uint16_t server_port;
    char device_name[SD_NAME_MAX + 1];
} sd_config_t;

/* Fill `out` with built-in defaults (empty credentials, default port/name). */
void sd_config_defaults(sd_config_t *out);

/* Load configuration from the storage backend. If nothing is stored yet, or the
 * stored version is incompatible, `out` is populated with defaults and SD_OK is
 * still returned (a fresh device is not an error). SD_ERR_IO on backend failure.
 */
sd_err_t sd_config_load(sd_config_t *out);

/* Persist `cfg` to the storage backend. */
sd_err_t sd_config_save(const sd_config_t *cfg);

/* String-keyed field access, shared by the config CLI and tests. Keys:
 *   "device_name", "wifi_ssid", "wifi_password", "server_host", "server_port".
 * set: validates length / numeric range; SD_ERR_INVALID on unknown key or bad
 * value. get: writes the value as a string into `out` (port as decimal);
 * SD_ERR_INVALID on unknown key. get returns the raw value — callers that
 * display it are responsible for masking secrets (see the CLI).
 */
sd_err_t sd_config_set_field(sd_config_t *cfg, const char *key, const char *value);
sd_err_t sd_config_get_field(const sd_config_t *cfg, const char *key, char *out,
                             size_t cap);

#endif /* SUDONIT_CONFIG_H */
