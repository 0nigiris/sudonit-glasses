#include "device.h"

#include "sudonit/hal/battery.h"
#include "sudonit/hal/camera.h"
#include "sudonit/log.h"

static const char *TAG = "device";

sd_err_t sd_device_init(void) {
    sd_err_t err = sd_camera_init();
    if (err != SD_OK) {
        SD_LOGE(TAG, "camera init failed: %s", sd_strerror(err));
        return err;
    }
    err = sd_battery_init();
    if (err != SD_OK) {
        SD_LOGE(TAG, "battery init failed: %s", sd_strerror(err));
        return err;
    }
    SD_LOGI(TAG, "initialized (camera=%s battery=%s)", sd_camera_backend(),
            sd_battery_backend());
    return SD_OK;
}

sd_err_t sd_device_capture_cycle(sd_device_status_t *status) {
    if (!status) {
        return SD_ERR_INVALID;
    }

    uint8_t percent = 0;
    sd_err_t err = sd_battery_read(&percent);
    if (err != SD_OK) {
        SD_LOGW(TAG, "battery read failed: %s", sd_strerror(err));
        return err;
    }

    sd_image_t img = {0};
    err = sd_camera_capture(&img);
    if (err != SD_OK) {
        SD_LOGE(TAG, "capture failed: %s", sd_strerror(err));
        return err;
    }

    SD_LOGI(TAG, "captured %ux%u %s (%zu bytes), battery %u%%", img.width,
            img.height, img.media_type, img.len, percent);

    status->image_bytes = img.len;
    status->image_width = img.width;
    status->image_height = img.height;
    status->battery_percent = percent;

    /* Next step (protocol component): hand img.data/len to the transport HAL
     * to stream to the phone. Released here for now. */
    sd_camera_release(&img);
    return SD_OK;
}
