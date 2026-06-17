/* ESP-IDF entry point.
 *
 * Proves the shared firmware code compiles and runs on the ESP32-S3 target: it
 * boots, reports the active HAL backends, and exercises the app -> HAL path.
 * The peripheral drivers in this build are honest stubs (SD_ERR_UNSUPPORTED),
 * so the capture cycle is expected to report an unsupported peripheral until
 * the real drivers land — that's the demonstration, not a failure.
 */
#include "nvs_flash.h"

#include "device.h"
#include "sudonit/config.h"
#include "sudonit/hal/audio.h"
#include "sudonit/hal/battery.h"
#include "sudonit/hal/camera.h"
#include "sudonit/hal/mic.h"
#include "sudonit/log.h"

static const char *TAG = "app_main";

void app_main(void) {
    sd_log_set_level(SD_LOG_DEBUG);

    SD_LOGI(TAG, "Sudonit Smart Glasses firmware — ESP-IDF target boot");
    SD_LOGI(TAG, "HAL backends: camera=%s battery=%s audio=%s mic=%s",
            sd_camera_backend(), sd_battery_backend(), sd_audio_backend(),
            sd_mic_backend());

    /* NVS backs the configuration store; initialize it before loading config. */
    esp_err_t nv = nvs_flash_init();
    if (nv == ESP_ERR_NVS_NO_FREE_PAGES || nv == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    sd_config_t cfg;
    sd_config_load(&cfg);
    /* Never log the password — report only whether it is set. */
    SD_LOGI(TAG, "config: device=%s server=%s:%u ssid=%s password=%s",
            cfg.device_name,
            cfg.server_host[0] ? cfg.server_host : "(unset)",
            (unsigned)cfg.server_port,
            cfg.wifi_ssid[0] ? cfg.wifi_ssid : "(unset)",
            cfg.wifi_password[0] ? "(set)" : "(unset)");

    sd_err_t err = sd_device_init();
    SD_LOGI(TAG, "device init: %s", sd_strerror(err));

    /* Exercise the app->HAL path. Stubs return SD_ERR_UNSUPPORTED — expected. */
    sd_device_status_t status = {0};
    err = sd_device_capture_cycle(&status);
    SD_LOGI(TAG, "capture cycle: %s", sd_strerror(err));

    SD_LOGI(TAG, "boot complete — peripheral drivers are stubs pending hardware");
}
