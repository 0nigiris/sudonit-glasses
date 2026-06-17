/* Configuration: defaults, serialization, versioned load/save.
 *
 * The on-wire/on-storage blob is [uint32 version][sd_config_t bytes]. The blob
 * is only ever read back by the same firmware build that wrote it, so raw-struct
 * persistence is safe; the version guard makes a future schema change explicit.
 */
#include "sudonit/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sudonit/config_store.h"
#include "sudonit/log.h"

static const char *TAG = "config";

#define VERSION_BYTES sizeof(uint32_t)
#define BLOB_BYTES (VERSION_BYTES + sizeof(sd_config_t))

void sd_config_defaults(sd_config_t *out) {
    if (!out) {
        return;
    }
    memset(out, 0, sizeof(*out));
    /* Credentials and server host are empty until provisioned. */
    out->server_port = 8765;

    const char *name = "sudonit-glasses";
    size_t n = strlen(name);
    if (n > SD_NAME_MAX) n = SD_NAME_MAX;
    memcpy(out->device_name, name, n);
    out->device_name[n] = '\0';
}

sd_err_t sd_config_load(sd_config_t *out) {
    if (!out) {
        return SD_ERR_INVALID;
    }

    uint8_t blob[BLOB_BYTES];
    size_t len = 0;
    sd_err_t err = sd_config_store_read(blob, sizeof(blob), &len);
    if (err == SD_ERR_NO_MEM) {
        /* Stored blob larger than our schema — an incompatible/corrupt version.
         * Fall back to defaults so the device still boots. */
        SD_LOGW(TAG, "stored config larger than expected; using defaults");
        sd_config_defaults(out);
        return SD_OK;
    }
    if (err != SD_OK) {
        SD_LOGW(TAG, "store read failed (%s); using defaults", sd_strerror(err));
        sd_config_defaults(out);
        return err;
    }

    if (len == 0) {
        SD_LOGI(TAG, "no stored config; using defaults");
        sd_config_defaults(out);
        return SD_OK;
    }

    uint32_t version = 0;
    if (len < BLOB_BYTES) {
        SD_LOGW(TAG, "stored config too small (%zu bytes); using defaults", len);
        sd_config_defaults(out);
        return SD_OK;
    }
    memcpy(&version, blob, VERSION_BYTES);
    if (version != SD_CONFIG_VERSION) {
        /* Future: migrate older versions here. For now, fall back to defaults. */
        /* Cast for portable printf: uint32_t is `unsigned long` on Xtensa but
         * `unsigned int` on the host; (unsigned) is correct for both. */
        SD_LOGW(TAG, "config version %u != %u; using defaults",
                (unsigned)version, (unsigned)SD_CONFIG_VERSION);
        sd_config_defaults(out);
        return SD_OK;
    }

    memcpy(out, blob + VERSION_BYTES, sizeof(sd_config_t));
    /* Defensive: guarantee NUL termination of string fields. */
    out->wifi_ssid[SD_WIFI_SSID_MAX] = '\0';
    out->wifi_password[SD_WIFI_PASS_MAX] = '\0';
    out->server_host[SD_HOST_MAX] = '\0';
    out->device_name[SD_NAME_MAX] = '\0';
    return SD_OK;
}

sd_err_t sd_config_save(const sd_config_t *cfg) {
    if (!cfg) {
        return SD_ERR_INVALID;
    }
    uint8_t blob[BLOB_BYTES];
    uint32_t version = SD_CONFIG_VERSION;
    memcpy(blob, &version, VERSION_BYTES);
    memcpy(blob + VERSION_BYTES, cfg, sizeof(sd_config_t));
    return sd_config_store_write(blob, sizeof(blob));
}

/* Copy `value` into a fixed string field, rejecting anything over `maxlen`. */
static sd_err_t set_str(char *dst, const char *value, size_t maxlen) {
    size_t n = strlen(value);
    if (n > maxlen) {
        return SD_ERR_INVALID;
    }
    memcpy(dst, value, n);
    dst[n] = '\0';
    return SD_OK;
}

sd_err_t sd_config_set_field(sd_config_t *cfg, const char *key, const char *value) {
    if (!cfg || !key || !value) {
        return SD_ERR_INVALID;
    }
    if (strcmp(key, "device_name") == 0) {
        return set_str(cfg->device_name, value, SD_NAME_MAX);
    }
    if (strcmp(key, "wifi_ssid") == 0) {
        return set_str(cfg->wifi_ssid, value, SD_WIFI_SSID_MAX);
    }
    if (strcmp(key, "wifi_password") == 0) {
        return set_str(cfg->wifi_password, value, SD_WIFI_PASS_MAX);
    }
    if (strcmp(key, "server_host") == 0) {
        return set_str(cfg->server_host, value, SD_HOST_MAX);
    }
    if (strcmp(key, "server_port") == 0) {
        char *end = NULL;
        long v = strtol(value, &end, 10);
        if (value[0] == '\0' || *end != '\0' || v < 1 || v > 65535) {
            return SD_ERR_INVALID;
        }
        cfg->server_port = (uint16_t)v;
        return SD_OK;
    }
    return SD_ERR_INVALID;
}

sd_err_t sd_config_get_field(const sd_config_t *cfg, const char *key, char *out,
                             size_t cap) {
    if (!cfg || !key || !out || cap == 0) {
        return SD_ERR_INVALID;
    }
    if (strcmp(key, "device_name") == 0) {
        snprintf(out, cap, "%s", cfg->device_name);
    } else if (strcmp(key, "wifi_ssid") == 0) {
        snprintf(out, cap, "%s", cfg->wifi_ssid);
    } else if (strcmp(key, "wifi_password") == 0) {
        snprintf(out, cap, "%s", cfg->wifi_password);
    } else if (strcmp(key, "server_host") == 0) {
        snprintf(out, cap, "%s", cfg->server_host);
    } else if (strcmp(key, "server_port") == 0) {
        snprintf(out, cap, "%u", (unsigned)cfg->server_port);
    } else {
        return SD_ERR_INVALID;
    }
    return SD_OK;
}
