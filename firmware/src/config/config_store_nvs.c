/* ESP-IDF configuration store: NVS blob.
 *
 * Stores the config blob under namespace "sudonit", key "config". A missing key
 * means "nothing stored yet" (*out_len == 0, not an error). The caller must have
 * initialized NVS (nvs_flash_init) before use — app_main does this at boot.
 *
 * This is a real ESP-IDF backend (NVS is available without external peripherals),
 * so it is implemented for real rather than stubbed.
 */
#include "sudonit/config_store.h"

#include "nvs.h"
#include "nvs_flash.h"

#define NVS_NAMESPACE "sudonit"
#define NVS_KEY "config"

sd_err_t sd_config_store_read(void *buf, size_t cap, size_t *out_len) {
    if (!buf || !out_len) {
        return SD_ERR_INVALID;
    }
    *out_len = 0;

    nvs_handle_t h;
    esp_err_t e = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (e == ESP_ERR_NVS_NOT_FOUND) {
        return SD_OK; /* namespace not created yet — fresh device */
    }
    if (e != ESP_OK) {
        return SD_ERR_IO;
    }

    size_t required = 0;
    e = nvs_get_blob(h, NVS_KEY, NULL, &required);
    if (e == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(h);
        return SD_OK; /* key not set yet */
    }
    if (e != ESP_OK) {
        nvs_close(h);
        return SD_ERR_IO;
    }
    if (required > cap) {
        nvs_close(h);
        return SD_ERR_NO_MEM;
    }

    e = nvs_get_blob(h, NVS_KEY, buf, &required);
    nvs_close(h);
    if (e != ESP_OK) {
        return SD_ERR_IO;
    }
    *out_len = required;
    return SD_OK;
}

sd_err_t sd_config_store_write(const void *buf, size_t len) {
    if (!buf && len > 0) {
        return SD_ERR_INVALID;
    }
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        return SD_ERR_IO;
    }
    sd_err_t result = SD_OK;
    if (nvs_set_blob(h, NVS_KEY, buf, len) != ESP_OK ||
        nvs_commit(h) != ESP_OK) {
        result = SD_ERR_IO;
    }
    nvs_close(h);
    return result;
}
