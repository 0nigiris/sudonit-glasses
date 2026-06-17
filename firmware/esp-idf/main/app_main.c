/* ESP-IDF entry point.
 *
 * Proves the shared firmware code compiles and runs on the ESP32-S3 target: it
 * boots, reports the active HAL backends, and exercises the app -> HAL path.
 * The peripheral drivers in this build are honest stubs (SD_ERR_UNSUPPORTED),
 * so the capture cycle is expected to report an unsupported peripheral until
 * the real drivers land — that's the demonstration, not a failure.
 */
#include "device.h"
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

    sd_err_t err = sd_device_init();
    SD_LOGI(TAG, "device init: %s", sd_strerror(err));

    /* Exercise the app->HAL path. Stubs return SD_ERR_UNSUPPORTED — expected. */
    sd_device_status_t status = {0};
    err = sd_device_capture_cycle(&status);
    SD_LOGI(TAG, "capture cycle: %s", sd_strerror(err));

    SD_LOGI(TAG, "boot complete — peripheral drivers are stubs pending hardware");
}
