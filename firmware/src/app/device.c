/* clock_gettime(CLOCK_MONOTONIC) for uplink latency instrumentation. */
#define _POSIX_C_SOURCE 200809L

#include "device.h"

#include <stdlib.h>
#include <time.h>

#include "sudonit/hal/audio.h"
#include "sudonit/hal/battery.h"
#include "sudonit/hal/camera.h"
#include "sudonit/log.h"
#include "sudonit/protocol/json.h"
#include "sudonit/protocol/messages.h"

static const char *TAG = "device";

/* Monotonic milliseconds for latency measurement. CLOCK_MONOTONIC exists on both
 * the host (POSIX) and the ESP32 (newlib), so the same instrumentation runs
 * unchanged on hardware. Stage deltas stay correct across the 32-bit wrap. */
static uint32_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint32_t)ts.tv_sec * 1000u + (uint32_t)(ts.tv_nsec / 1000000));
}

/* Close out the timing for a turn that reached the end (response + total). */
static void finalize_metrics(sd_uplink_metrics_t *m, uint32_t t_start,
                             uint32_t t_upload_done) {
    if (!m) {
        return;
    }
    uint32_t now = now_ms();
    m->response_ms = now - t_upload_done;
    m->total_ms = now - t_start;
}

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

/* Receive the audio downlink described by `begin_json` and play it through the
 * audio HAL. The PCM is heap-buffered (a full utterance can be tens of KB) and
 * freed before returning. Returns SD_OK once the utterance has been played. */
static sd_err_t play_audio_downlink(sd_transport_t *t, const char *begin_json) {
    uint32_t size = 0, chunks = 0, sample_rate = 0, channels = 0;
    char sha[65];
    if (sd_json_get_uint(begin_json, "size", &size) != SD_OK ||
        sd_json_get_uint(begin_json, "chunks", &chunks) != SD_OK ||
        sd_json_get_uint(begin_json, "sample_rate", &sample_rate) != SD_OK ||
        sd_json_get_uint(begin_json, "channels", &channels) != SD_OK ||
        sd_json_get_string(begin_json, "sha256", sha, sizeof(sha)) != SD_OK) {
        SD_LOGE(TAG, "uplink: malformed audio_begin");
        return SD_ERR_IO;
    }
    if (channels == 0) {
        return SD_ERR_IO;
    }

    uint8_t *pcm = malloc(size ? size : 1);
    if (!pcm) {
        return SD_ERR_NO_MEM;
    }
    sd_err_t err = sd_msg_recv_audio_body(t, chunks, size, sha, pcm, size);
    if (err != SD_OK) {
        SD_LOGE(TAG, "uplink: audio body failed: %s", sd_strerror(err));
        free(pcm);
        return err;
    }

    err = sd_audio_init(sample_rate, (uint8_t)channels);
    if (err == SD_OK) {
        size_t frames = (size_t)size / (2u * channels); /* 16-bit samples */
        err = sd_audio_play_pcm((const int16_t *)pcm, frames);
        SD_LOGI(TAG, "uplink: played %zu audio frames (%u Hz, %u ch)", frames,
                (unsigned)sample_rate, (unsigned)channels);
    } else {
        SD_LOGW(TAG, "uplink: audio unavailable (%s); discarding PCM",
                sd_strerror(err));
    }
    free(pcm);
    return err;
}

sd_err_t sd_device_run_uplink(sd_transport_t *t, const char *image_id,
                              char *response_out, size_t response_cap,
                              sd_uplink_metrics_t *metrics) {
    if (!t || !image_id) {
        return SD_ERR_INVALID;
    }
    if (response_out && response_cap > 0) {
        response_out[0] = '\0';
    }
    if (metrics) {
        *metrics = (sd_uplink_metrics_t){0};
    }

    uint32_t t_start = now_ms();
    sd_image_t img = {0};
    sd_err_t err = sd_camera_capture(&img);
    if (err != SD_OK) {
        SD_LOGE(TAG, "uplink capture failed: %s", sd_strerror(err));
        return err;
    }
    uint32_t t_capture_done = now_ms();
    SD_LOGI(TAG, "uplink: captured %ux%u %s (%zu bytes)", img.width, img.height,
            img.media_type, img.len);
    if (metrics) {
        metrics->capture_ms = t_capture_done - t_start;
        metrics->image_bytes = img.len;
    }

    err = sd_msg_send_image(t, image_id, img.data, img.len, img.media_type);
    sd_camera_release(&img);
    if (err != SD_OK) {
        SD_LOGE(TAG, "uplink send failed: %s", sd_strerror(err));
        return err;
    }
    uint32_t t_upload_done = now_ms();
    if (metrics) {
        metrics->upload_ms = t_upload_done - t_capture_done;
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
            if (metrics) {
                metrics->response_bytes = 0;
                while (text[metrics->response_bytes]) {
                    metrics->response_bytes++;
                }
            }
            if (response_out && response_cap > 0) {
                /* Hand the AI text back to the application layer. */
                size_t i = 0;
                for (; text[i] && i + 1 < response_cap; ++i) {
                    response_out[i] = text[i];
                }
                response_out[i] = '\0';
            }
        } else if (type == SD_MSG_AUDIO_BEGIN) {
            /* The phone is streaming rendered speech as PCM: receive and play it.
             * This is the end of the turn. */
            err = play_audio_downlink(t, text);
            finalize_metrics(metrics, t_start, t_upload_done);
            return err;
        } else if (type == SD_MSG_PLAY_AUDIO) {
            SD_LOGI(TAG, "uplink: play_audio (turn complete)");
            finalize_metrics(metrics, t_start, t_upload_done);
            return SD_OK;
        } else if (type == SD_MSG_ERROR) {
            SD_LOGE(TAG, "uplink: phone error: %s", text);
            return SD_ERR_IO;
        }
        /* pong/device_info/unknown: ignore and keep reading. */
    }
}
