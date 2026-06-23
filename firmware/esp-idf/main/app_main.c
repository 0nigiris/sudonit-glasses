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
#include "sudonit/hal/net.h"
#include "sudonit/log.h"
#include "sudonit/provisioning.h"

/* The network data-plane code below is shared by two opt-in build flags:
 *   SUDONIT_NET_SELFTEST - bring Wi-Fi up and exchange ping/pong (liveness).
 *   SUDONIT_RUN_UPLINK   - bring Wi-Fi up and run the full capture->AI->audio
 *                          turn via sd_device_run_uplink (the real V1 loop).
 * Both reuse the same bring-up + bounded-retry connect; they differ only in what
 * they do once connected. */
#if defined(SUDONIT_NET_SELFTEST) || defined(SUDONIT_RUN_UPLINK)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sudonit/hal/transport.h"
#include "sudonit/protocol/messages.h"
#endif

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

#if defined(SUDONIT_NET_SELFTEST) || defined(SUDONIT_RUN_UPLINK)
    /* Data plane: bring Wi-Fi up from the provisioned credentials and open a TCP
     * connection to the phone server. This exercises net_esp + transport_wifi +
     * the protocol layer end-to-end. With SUDONIT_NET_SELFTEST we then exchange
     * ping/pong (no camera needed); with SUDONIT_RUN_UPLINK we run the full
     * capture->AI->audio turn — the same sd_device_run_uplink proven on the host
     * interop build. Until the real camera driver is enabled
     * (-DSUDONIT_CAMERA_DRIVER=1) the uplink's capture step returns
     * SD_ERR_UNSUPPORTED and the turn ends there: the wiring is in place and
     * compiled, so hardware day is flash-and-run rather than integrate. */
    SD_LOGI(TAG, "data plane: bringing up Wi-Fi (net backend=%s)", sd_net_backend());
    if (sd_net_start(&cfg) != SD_OK) {
        SD_LOGE(TAG, "net self-test: Wi-Fi bring-up failed");
    } else if (cfg.server_host[0] == '\0') {
        SD_LOGE(TAG, "net self-test: no server_host configured");
    } else {
        /* Reconnect with backoff: at bring-up the phone app may not be listening
         * yet, and Wi-Fi/TCP can drop. Retry a bounded number of times rather
         * than failing on the first attempt. Each connect attempt is itself
         * bounded by the transport's connect timeout. */
        sd_transport_t *t = NULL;
        sd_err_t terr = SD_ERR_IO;
        for (int attempt = 1; attempt <= 5; ++attempt) {
            terr = sd_transport_connect(&t, cfg.server_host, cfg.server_port);
            if (terr == SD_OK) {
                break;
            }
            SD_LOGW(TAG, "net self-test: connect attempt %d/5 to %s:%u failed: %s",
                    attempt, cfg.server_host, (unsigned)cfg.server_port,
                    sd_strerror(terr));
            vTaskDelay(pdMS_TO_TICKS(1000 * attempt)); /* linear backoff */
        }
        if (terr != SD_OK) {
            SD_LOGE(TAG, "data plane: could not connect after retries");
        } else {
#ifdef SUDONIT_RUN_UPLINK
            /* Full V1 turn: capture -> stream to phone -> AI response -> play the
             * audio downlink. Identical code path to the host interop build. */
            char response[512];
            sd_uplink_metrics_t m = {0};
            sd_err_t uerr =
                sd_device_run_uplink(t, "capture", response, sizeof(response), &m);
            if (uerr == SD_OK) {
                SD_LOGI(TAG, "uplink: turn complete — response: %s", response);
                SD_LOGI(TAG, "uplink latency: capture=%ums upload=%ums "
                             "response=%ums total=%ums (image %zuB)",
                        (unsigned)m.capture_ms, (unsigned)m.upload_ms,
                        (unsigned)m.response_ms, (unsigned)m.total_ms,
                        m.image_bytes);
            } else {
                SD_LOGE(TAG, "uplink: turn failed: %s (capture is stubbed until "
                             "-DSUDONIT_CAMERA_DRIVER=1)",
                        sd_strerror(uerr));
            }
#else
            sd_err_t perr = sd_msg_send_ping(t);
            sd_msg_type_t mtype = SD_MSG_UNKNOWN;
            if (perr == SD_OK) {
                perr = sd_msg_recv(t, &mtype, NULL, 0);
            }
            if (perr == SD_OK && mtype == SD_MSG_PONG) {
                SD_LOGI(TAG, "net self-test: ping/pong OK — data plane is live");
            } else {
                SD_LOGE(TAG, "net self-test: no pong (%s, type=%d)",
                        sd_strerror(perr), (int)mtype);
            }
#endif
            sd_transport_close(t);
        }
    }
#endif

#ifdef SUDONIT_PROVISION_CONSOLE
    /* Recovery/provisioning channel: drop into the serial console over the UART.
     * ESP-IDF maps stdio to the console UART, so the same REPL used on the host
     * runs unchanged here. Guarded by a build flag so production boots skip it. */
    SD_LOGI(TAG, "entering provisioning console (SUDONIT_PROVISION_CONSOLE)");
    sd_provision_repl(stdin, stdout);
#endif
}
