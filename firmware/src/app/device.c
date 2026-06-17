#include "device.h"

#include "sudonit/hal/battery.h"
#include "sudonit/hal/camera.h"
#include "sudonit/log.h"
#include "sudonit/protocol/messages.h"

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

    sd_camera_release(&img);
    return SD_OK;
}

sd_err_t sd_device_run_uplink(sd_transport_t *t, const char *image_id,
                              char *response_out, size_t response_cap) {
    if (!t || !image_id) {
        return SD_ERR_INVALID;
    }
    if (response_out && response_cap > 0) {
        response_out[0] = '\0';
    }

    sd_image_t img = {0};
    sd_err_t err = sd_camera_capture(&img);
    if (err != SD_OK) {
        SD_LOGE(TAG, "uplink capture failed: %s", sd_strerror(err));
        return err;
    }
    SD_LOGI(TAG, "uplink: captured %ux%u %s (%zu bytes)", img.width, img.height,
            img.media_type, img.len);

    err = sd_msg_send_image(t, image_id, img.data, img.len, img.media_type);
    sd_camera_release(&img);
    if (err != SD_OK) {
        SD_LOGE(TAG, "uplink send failed: %s", sd_strerror(err));
        return err;
    }

    /* Read control replies until the phone signals playback (end of turn). */
    for (;;) {
        sd_msg_type_t type = SD_MSG_UNKNOWN;
        char text[1024];
        err = sd_msg_recv(t, &type, text, sizeof(text));
        if (err != SD_OK) {
            SD_LOGE(TAG, "uplink recv failed: %s", sd_strerror(err));
            return err;
        }
        if (type == SD_MSG_AI_RESPONSE) {
            SD_LOGI(TAG, "uplink: ai_response: %s", text);
            if (response_out && response_cap > 0) {
                /* Hand the AI text back to the application layer. */
                size_t i = 0;
                for (; text[i] && i + 1 < response_cap; ++i) {
                    response_out[i] = text[i];
                }
                response_out[i] = '\0';
            }
        } else if (type == SD_MSG_PLAY_AUDIO) {
            SD_LOGI(TAG, "uplink: play_audio (turn complete)");
            return SD_OK;
        } else if (type == SD_MSG_ERROR) {
            SD_LOGE(TAG, "uplink: phone error: %s", text);
            return SD_ERR_IO;
        }
        /* pong/device_info/unknown: ignore and keep reading. */
    }
}
